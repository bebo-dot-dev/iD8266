#include "webserver.h"
#include "flashAppData.h"
#include "build_defs.h"
#include "utils.h"
#include <ESP8266WebServer.h>
#include <FS.h>
#include "NetworkServices.h"
#include "SessionManager.h"
#include "ESP8266TOTP.h"
#include "Complexify.h"


ESP8266WebServer* webserver;

//a reference to the current SPIFFS file system uploaded file
File fsUploadFile;

#define lastModifiedTimestamp getAppStr(appStrType::lastModifiedTimestampStr) //"Fri, 04 Mar 2016 21:15:00 GMT" //an arbitrary point in time used for cache 304 responses

bool firmwareUploadFail = false;
unsigned long rebootTimeStamp = 0;
unsigned long lastHeapCheck = 0;

enum jsonResponseType
{
	networkSettings = 0,
	dnsSettings = 1,
	webserverSettings = 2,
	regionalSettings = 3,
	diagnosticsData = 4,
	mqttData = 5,
	gpioData = 6,
	homePageData = 7,
	powerMgmtData = 8,
	powerMgmtScheduleData = 9,
	securityData = 10,
	totpQrCodeUriData = 11,
	peripheralsData = 12
};

String ICACHE_FLASH_ATTR schemeAndDomain() {

	String schemeAndDomainStr =
		webserver->hasHeader(getAppStr(appStrType::sslReverseProxy))
		? getAppStr(appStrType::httpsScheme)
		: getAppStr(appStrType::httpScheme);

	if (webserver->hostHeader() != EMPTY_STR) {
		schemeAndDomainStr += webserver->hostHeader();
	} else {
		schemeAndDomainStr += WiFi.softAPIP().toString();
	}

	String portHeader = getAppStr(appStrType::sslReverseProxyWebserverPort);
	if(webserver->hasHeader(portHeader))
		schemeAndDomainStr += getAppStr(appStrType::colon) + webserver->header(portHeader);

	return schemeAndDomainStr;
}

/*
 * converts a session cookieVal string to an unsigned long sessionId value
 */
String ICACHE_FLASH_ATTR getSessionIdFromCookieVal(const String &cookieVal) {

	String sessionId = EMPTY_STR;
	if (cookieVal.indexOf(getAppStr(appStrType::sessionId)) != -1) {
		sessionId = cookieVal.substring(cookieVal.indexOf(getAppStr(appStrType::sessionLocator)) + SESSION_KEY_LENGTH, cookieVal.indexOf(getAppStr(appStrType::pipe)));
	}
	return sessionId;
}

/*
 * returns the value of the sessionId cookie for the current HTTP request
 */
String ICACHE_FLASH_ATTR getSessionCookieValFromHttpHeader() {

	String cookieStr = getAppStr(appStrType::cookie);
	String result = EMPTY_STR;
	bool hasCookie = webserver->hasHeader(cookieStr);
	if (hasCookie) {
		result = webserver->header(cookieStr);
	}
	return result;
}

/*
 * returns an indicator whether the sessionId cookie value represents an active session
 */
bool ICACHE_FLASH_ATTR isAuthenticated(const String &sessionCookieVal) {

	String cookieVal;
	if (sessionCookieVal.equals(EMPTY_STR)) {
		cookieVal = getSessionCookieValFromHttpHeader();
	} else {
		cookieVal = sessionCookieVal;
	}

	if (!cookieVal.equals(EMPTY_STR)) {
		cookieVal = getSessionIdFromCookieVal(cookieVal);
		bool isAuthed = !cookieVal.equals(EMPTY_STR) ? SessionMngr.webSessionExists(cookieVal) : false;
		return isAuthed;
	}
	return false;

}

void ICACHE_FLASH_ATTR includeDefaultHeaders() {
	if (appConfigData.includeServerHeader) {
		webserver->sendHeader(getAppStr(appStrType::serverStr), appConfigData.serverHeader);
	}
	String cachePolicy =
			webserver->hasHeader(getAppStr(appStrType::sslReverseProxy))
			? getAppStr(appStrType::cacheControlPolicy300MaxAge) //specify a max age of 300 (seconds) when proxied to enable some caching
			: getAppStr(appStrType::cacheControlPolicyZeroMaxAge); //local browser seems perform well enough revalidating via If-Modified-Since

	webserver->sendHeader(getAppStr(appStrType::cacheControl), cachePolicy);
	webserver->sendHeader(getAppStr(appStrType::lastModified), lastModifiedTimestamp);
}

void ICACHE_FLASH_ATTR includeNoCacheHeaders() {
	if (appConfigData.includeServerHeader) {
		webserver->sendHeader(getAppStr(appStrType::serverStr), appConfigData.serverHeader);
	}
	webserver->sendHeader(getAppStr(appStrType::cacheControl), getAppStr(appStrType::noCacheControlPolicy));
}

String ICACHE_FLASH_ATTR getContentType(const String &filename) {
	if (filename.endsWith(getAppStr(appStrType::htmExtension)))
		return getAppStr(appStrType::htmlContentType);
	else if (filename.endsWith(getAppStr(appStrType::htmlExtension)))
		return getAppStr(appStrType::htmlContentType);
	else if (filename.endsWith(getAppStr(appStrType::cssExtension)))
		return getAppStr(appStrType::cssContentType);
	else if (filename.endsWith(getAppStr(appStrType::jsExtension)))
		return getAppStr(appStrType::jsContentType);
	else if (filename.endsWith(getAppStr(appStrType::pngExtension)))
		return getAppStr(appStrType::pngContentType);
	else if (filename.endsWith(getAppStr(appStrType::gifExtension)))
		return getAppStr(appStrType::gifContentType);
	else if (filename.endsWith(getAppStr(appStrType::jpgExtension)))
		return getAppStr(appStrType::jpegContentType);
	else if (filename.endsWith(getAppStr(appStrType::icoExtension)))
		return getAppStr(appStrType::icoContentType);
	else if (filename.endsWith(getAppStr(appStrType::xmlExtension)))
		return getAppStr(appStrType::xmlContentType);
	else if (filename.endsWith(getAppStr(appStrType::pdfExtension)))
		return getAppStr(appStrType::pdfContentType);
	else if (filename.endsWith(getAppStr(appStrType::zipExtension)))
		return getAppStr(appStrType::zipContentType);
	else if (filename.endsWith(getAppStr(appStrType::gzExtension)))
		return getAppStr(appStrType::gzipContentType);
	return getAppStr(appStrType::textContentType);
}

bool ICACHE_FLASH_ATTR returnStreamedFile(const String &path, const String &contentType) {

	String acceptEncodingStr = getAppStr(appStrType::acceptEncoding);
	String ifModifiedSinceStr = getAppStr(appStrType::ifModifiedSince);

	bool acceptEncodingHdrPresent = webserver->hasHeader(acceptEncodingStr);
	String acceptEncodingHdrVal = webserver->header(acceptEncodingStr);
	bool ifModifiedSinceHdrPresent = webserver->hasHeader(ifModifiedSinceStr);
	String ifModifiedSinceHdrVal = ifModifiedSinceHdrPresent
		? webserver->header(ifModifiedSinceStr)
		: EMPTY_STR;

	if (ifModifiedSinceHdrPresent) {
		if (ifModifiedSinceHdrVal.equals(lastModifiedTimestamp)) {
			//return a 304 not modified response
			includeDefaultHeaders();
			webserver->send(304, contentType, EMPTY_STR);
			return true;
		}
	}

	bool gzipEnabled = ((acceptEncodingHdrPresent) && (acceptEncodingHdrVal.indexOf(getAppStr(appStrType::gzipStr))) != -1);
	String pathWithGz = path + getAppStr(appStrType::gzExtension);
	bool gzipFileExists = false;
	if (gzipEnabled) {
		gzipFileExists = SPIFFS.exists(pathWithGz);
	}

	if (gzipEnabled && gzipFileExists) {

		includeDefaultHeaders();
		yield();

		File file = SPIFFS.open(pathWithGz, "r");
		//stream back to client
		webserver->streamFile(file, contentType);
		file.close();
		return true;
	}

	return false;

}

bool ICACHE_FLASH_ATTR returnResource(const String &path, bool requiresAuth = true) {

	String contentType = getContentType(path);

	bool isAuthed = requiresAuth ? isAuthenticated() : true;
	bool hasDefaultPwd = (strcmp(DEFAULT_PWD, appConfigData.adminPwd) == 0);
	if (requiresAuth && !isAuthed && path.indexOf(getAppStr(appStrType::loginMin)) == -1 && path.indexOf(getAppStr(appStrType::logonStr)) == -1) {

		//return a 301 redirect to the logon resource content
		redirect(getAppStr(appStrType::loginStr), contentType);
		return true;

	}
	else if (requiresAuth && isAuthed && hasDefaultPwd && path.indexOf(getAppStr(appStrType::password)) == -1) {

		//return a 301 redirect to the change password resource content
		redirect(getAppStr(appStrType::passwordStr), contentType);
		return true;

	}
	else {

		bool result = returnStreamedFile(path, contentType);
		return result;

	}

	return false;
}

//writes current flash configuration data and either reboots the device or sends a JSON response
void ICACHE_FLASH_ATTR persistConfig(bool reboot = true) {

	FlashAppDataMngr.setAppData();

	if(reboot) {

		returnResource(getAppStr(appStrType::updateDone));
		rebootTimeStamp = millis() + 1000;

	} else {

		webserver->send(200, getAppStr(appStrType::jsonContentType), getAppStr(appStrType::jsonResultOk));

	}

}

void ICACHE_FLASH_ATTR handleFirmwareUpload() {

	if (isAuthenticated()) {
		// handler for firmware file upload and flash
		// firmware flash is handled via the UpdaterClass.
		// See Updater.h in the core for details

		HTTPUpload& upload = webserver->upload();

		if (upload.status == UPLOAD_FILE_START) {

			WiFiUDP::stopAll();

			APP_SERIAL_DEBUG("Firmware update: %s\n", upload.filename.c_str());

			firmwareUploadFail = false;

			if (!upload.filename.endsWith(getAppStr(appStrType::binExtension))) {
				firmwareUploadFail = true;
				return;
			}

			uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
			if (!Update.begin(maxSketchSpace)) {  //start with max available size
				Update.printError(Serial);
			}

			NetworkSvcMngr.powerManager->isDisabled = true;

		} else if (upload.status == UPLOAD_FILE_WRITE) {

			APP_SERIAL_DEBUG(".");
			if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
				Update.printError(Serial);
			}
			yield();

		} else if (upload.status == UPLOAD_FILE_END) {

			if (Update.end(true)) { //true to set the size to the current progress
				APP_SERIAL_DEBUG("Firmware update Success\nUpload size: %d\nRebooting...", upload.totalSize);
			} else {
				Update.printError(Serial);
			}

		} else if (upload.status == UPLOAD_FILE_ABORTED) {

			NetworkSvcMngr.powerManager->isDisabled = false;
			Update.end();
			APP_SERIAL_DEBUG("Firmware update was aborted\n");

		}

		yield();

	} else {

		redirect(getAppStr(appStrType::loginStr), getAppStr(appStrType::htmlContentType));

	}
}

/*
 * returns an indicator whether the totp parameter passed to the server matches the current otp
 * calculated from the epoch and current secret key bytes
 */
bool ICACHE_FLASH_ATTR isTotpTokenValid() {

	String argName = getAppStr(appStrType::totp);
	if (webserver->hasArg(argName)) {
		String suppliedTotp = webserver->arg(argName);

		char *endptr;
		int suppliedOtp = strtoul(suppliedTotp.c_str(), &endptr, 10);

		//check the supplied OTP against the current secret key
		return ((*endptr == '\0') &&
				(ESP8266TOTP::IsTokenValid(
						NetworkSvcMngr.ntpManager->getUtcTimeNow(),
						appConfigData.otp.keyBytes,
						suppliedOtp)));
	} else {
		return false;
	}

}

/*
 * handles a logon attempt. returns a valid session and redirects if successful
 */
void ICACHE_FLASH_ATTR handleLogon() {

	String argName = getAppStr(appStrType::pwd);

	if (webserver->hasArg(argName)) {

		String pwd = webserver->arg(argName);

		String sessionId = EMPTY_STR;
		bool pwdMatch = (strcmp(pwd.c_str(), appConfigData.adminPwd) == 0);
		if (pwdMatch) {

			if ((appConfigData.otp.enabled) &&
				(NetworkSvcMngr.ntpEnabled) &&
				(NetworkSvcMngr.ntpManager->syncResponseReceived)) {

				//check the supplied OTP against the persisted secret key
				if (isTotpTokenValid()) {
					sessionId = SessionMngr.addWebSession();
				}

			} else {
				sessionId = SessionMngr.addWebSession();
			}
		}

		if ((pwdMatch) && (!sessionId.equals(EMPTY_STR))) {

			//create and return a new session cookie
			char sessionCookieStr[SESSION_KEY_LENGTH + SESSION_UUID_LENGTH];
			sprintf(sessionCookieStr, getAppStr(appStrType::sessionPlaceholder).c_str(), sessionId.c_str());
			webserver->sendHeader(getAppStr(appStrType::setCookie), sessionCookieStr);

			//return a 301 redirect to the root /
			redirect(getAppStr(appStrType::FORWARD_SLASH), getAppStr(appStrType::htmlContentType));

		} else {

			//access denied
			returnResource(getAppStr(appStrType::slashLoginMin), false);

		}
	}
}

void ICACHE_FLASH_ATTR handleLogoff() {

	if (isAuthenticated()) {

		String sessionCookieVal = getSessionCookieValFromHttpHeader();

		if (sessionCookieVal != EMPTY_STR) {
			String sessionId = getSessionIdFromCookieVal(sessionCookieVal);
			SessionMngr.freeSession(sessionId);
		}

		//session cookie wipeout and redirect to logon
		webserver->sendHeader(getAppStr(appStrType::setCookie), getAppStr(appStrType::resetCookie));
		redirect(getAppStr(appStrType::loginStr), getAppStr(appStrType::htmlContentType));

	} else {

		redirect(getAppStr(appStrType::loginStr), getAppStr(appStrType::htmlContentType));

	}
}

/*
 returns a json string that looks like this:
 {
 wifiMode : '1',
 apSSID : 'deviceApSsid1234',
 apPwd : 'apPassword1234',
 apChannel : '5',
 netApSSID : 'netApSsid1234',
 netApPwd : 'netApPwd1234',
 netApIpMode : '1',
 netApStaticIp : '192.168.0.15',
 netApSubnet : '255.255.255.0',
 netApGatewayIp : '192.168.0.1',
 netApDnsIp : '8.8.8.8'
 }
*/
String ICACHE_FLASH_ATTR getNetSettings() {

	char cstr[2];
	String aStr;

	String intPlaceholder = getAppStr(appStrType::intPlaceholderStr);
	String escapedCommaChar = getAppStr(appStrType::escapedComma);

	String json = EMPTY_STR;
	json += getAppStr(appStrType::openBrace);

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.wifiMode);
	aStr = String(cstr);

	json += getAppStr(appStrType::escapedWifiMode) + aStr + escapedCommaChar;
	aStr = String(appConfigData.deviceApSSID);
	json += getAppStr(appStrType::escapedApSSID) + aStr + escapedCommaChar;
	aStr = String(appConfigData.deviceApPwd);
	json += getAppStr(appStrType::escapedApPwd) + aStr + escapedCommaChar;

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.deviceApChannel);
	aStr = String(cstr);

	json += getAppStr(appStrType::escapedApChannel) + aStr + escapedCommaChar;
	aStr = String(appConfigData.networkApSSID);
	json += getAppStr(appStrType::escapedNetApSSID) + aStr + escapedCommaChar;
	aStr = String(appConfigData.networkApPwd);
	json += getAppStr(appStrType::escapedNetApPwd) + aStr + escapedCommaChar;

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.networkApIpMode);
	aStr = String(cstr);

	json += getAppStr(appStrType::escapedNetApIpMode) + aStr + escapedCommaChar;

	char ipcstr[16];
	String ipAddrPlaceholder = getAppStr(appStrType::ipAddressTemplateStr);

	sprintf(ipcstr, ipAddrPlaceholder.c_str(),
			appConfigData.networkApStaticIp[0],
			appConfigData.networkApStaticIp[1],
			appConfigData.networkApStaticIp[2],
			appConfigData.networkApStaticIp[3]);
	aStr = String(ipcstr);
	json += getAppStr(appStrType::escapedNetApStaticIp) + aStr + escapedCommaChar;

	sprintf(ipcstr, ipAddrPlaceholder.c_str(),
			appConfigData.networkApSubnet[0],
			appConfigData.networkApSubnet[1],
			appConfigData.networkApSubnet[2],
			appConfigData.networkApSubnet[3]);
	aStr = String(ipcstr);
	json += getAppStr(appStrType::escapedNetApSubnet) + aStr + escapedCommaChar;

	sprintf(ipcstr, ipAddrPlaceholder.c_str(),
			appConfigData.networkApGatewayIp[0],
			appConfigData.networkApGatewayIp[1],
			appConfigData.networkApGatewayIp[2],
			appConfigData.networkApGatewayIp[3]);
	aStr = String(ipcstr);
	json += getAppStr(appStrType::escapedNetApGatewayIp) + aStr + escapedCommaChar;

	sprintf(ipcstr, ipAddrPlaceholder.c_str(),
			appConfigData.networkApDnsIp[0],
			appConfigData.networkApDnsIp[1],
			appConfigData.networkApDnsIp[2],
			appConfigData.networkApDnsIp[3]);
	aStr = String(ipcstr);
	json += getAppStr(appStrType::escapedNetApDnsIp) + aStr + escapedCommaChar;

	sprintf(ipcstr, ipAddrPlaceholder.c_str(),
			appConfigData.networkApDns2Ip[0],
			appConfigData.networkApDns2Ip[1],
			appConfigData.networkApDns2Ip[2],
			appConfigData.networkApDns2Ip[3]);
	aStr = String(ipcstr);
	json += getAppStr(appStrType::escapedNetApDns2Ip) + aStr + getAppStr(appStrType::escapedQuote);

	json += getAppStr(appStrType::closeBrace);

	return json;

}

/*
 returns a json string that looks like this:
 {
 hostName : 'hostName',
 mdnsEnabled : 'true',
 dnsEnabled : 'true',
 dnsTTL : '300',
 dnsPort : '53',
 dnsCatchAll : 'false'
 }
*/
String ICACHE_FLASH_ATTR getDnsSettings() {

	char cstr[5];
	String aStr;

	String escapedCommaChar = getAppStr(appStrType::escapedComma);
	String intPlaceholder = getAppStr(appStrType::intPlaceholderStr);

	String json = EMPTY_STR;
	json += getAppStr(appStrType::openBrace);

	aStr = String(appConfigData.hostName);
	json += getAppStr(appStrType::escapedHostName) + aStr + escapedCommaChar;

	appConfigData.mdnsEnabled ?
		json += getAppStr(appStrType::mdnsEnabledTrue) :
		json += getAppStr(appStrType::mdnsEnabledFalse);

	appConfigData.dnsEnabled ?
		json += getAppStr(appStrType::dnsEnabledTrue) :
		json += getAppStr(appStrType::dnsEnabledFalse);

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.dnsTTL);
	aStr = String(cstr);
	json += getAppStr(appStrType::escapedDnsTTL) + aStr + escapedCommaChar;

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.dnsPort);
	aStr = String(cstr);
	json += getAppStr(appStrType::escapedDnsPort) + aStr + escapedCommaChar;

	appConfigData.dnsCatchAll ?
		json += getAppStr(appStrType::dnsCatchAllTrue) :
		json += getAppStr(appStrType::dnsCatchAllFalse);

	json += getAppStr(appStrType::closeBrace);

	return json;

}

/*
 returns a json string that looks like this:
 {
 'adminPwd' : 'password1234',
 'webserverPort' : '80',
 'websocketServerPort' : '81',
 'includeServerHeader' : 'false',
 'serverHeader' : 'serverName'
 }
*/
String ICACHE_FLASH_ATTR getWebserverSettings() {

	char cstr[5];
	String aStr;

	String escapedCommaChar = getAppStr(appStrType::escapedComma);
	String intPlaceholder = getAppStr(appStrType::intPlaceholderStr);
	String escapedQuoteChar = getAppStr(appStrType::escapedQuote);

	String json = getAppStr(appStrType::openBrace);

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.webserverPort);
	aStr = String(cstr);
	json += getAppStr(appStrType::escapedWebserverPort) + aStr + escapedCommaChar;

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.websocketServerPort);
	aStr = String(cstr);
	json += getAppStr(appStrType::escapedWebsocketServerPort) + aStr + escapedCommaChar;

	appConfigData.includeServerHeader ?
		json += getAppStr(appStrType::includeServerHeaderTrue) :
		json += getAppStr(appStrType::includeServerHeaderFalse);

	aStr = String(appConfigData.serverHeader);
	json += getAppStr(appStrType::escapedServerHeader) + aStr + getAppStr(appStrType::escapedQuote);

	webserver->hasHeader(getAppStr(appStrType::sslReverseProxy)) ?
		json += getAppStr(appStrType::sslProxyEnabledTrue) :
		json += getAppStr(appStrType::sslProxyEnabledFalse);

	aStr = getAppStr(appStrType::sslReverseProxyWebserverPort);
	if (webserver->hasHeader(aStr)) {
		json += getAppStr(appStrType::sslProxyWebserverPort) + webserver->header(aStr) + escapedQuoteChar;
	}

	aStr = getAppStr(appStrType::sslReverseProxyWebsocketPort);
	if (webserver->hasHeader(aStr)) {
		json += getAppStr(appStrType::sslProxyWebsocketPort) + webserver->header(aStr) + escapedQuoteChar;
	}

	json += getAppStr(appStrType::closeBrace);

	return json;
}

/*
 returns a json string that looks like this:
 {
 'ntpEnabled' : 'false',
 'ntpServer' : '0',
 'ntpTimeZone' : '0',
 'ntpSyncInterval' : '43200',
 'syncResponseReceived' : 'false',
 'year' : '0',
 'month' : '0',
 'day' : '0',
 'hour' : '0',
 'minute' : '0',
 'second' : '0',
 'lastSyncYear' : '0',
 'lastSyncMonth' : '0',
 'lastSyncDay' : '0',
 'lastSyncHour' : '0',
 'lastSyncMinute' : '0',
 'lastSyncSecond' : '0'
 }
*/
String ICACHE_FLASH_ATTR getRegionalSettings() {

	char cstr[5];
	String aStr;

	String escapedCommaChar = getAppStr(appStrType::escapedComma);
	String intPlaceholder = getAppStr(appStrType::intPlaceholderStr);

	String json = EMPTY_STR;
	json += getAppStr(appStrType::openBrace);

	appConfigData.ntpEnabled ?
		json += getAppStr(appStrType::ntpEnabledTrue) :
		json += getAppStr(appStrType::ntpEnabledFalse);

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.ntpServer);
	aStr = String(cstr);
	json += getAppStr(appStrType::escapedNtpServer) + aStr + escapedCommaChar;

	sprintf(cstr, intPlaceholder.c_str(), appConfigData.ntpTimeZone);
	aStr = String(cstr);
	json += getAppStr(appStrType::escapedNtpTimeZone) + aStr + escapedCommaChar;

	sprintf(cstr, "%ld", appConfigData.ntpSyncInterval);
	aStr = String(cstr);
	json += getAppStr(appStrType::escapedNtpSyncInterval) + aStr + escapedCommaChar;

	aStr = String(appConfigData.ntpLocale);
	json += getAppStr(appStrType::escapedNtpLocale) + aStr + getAppStr(appStrType::escapedQuote);

	if (NetworkSvcMngr.ntpEnabled) {

		json += getAppStr(appStrType::comma);

		NetworkSvcMngr.ntpManager->syncResponseReceived ?
			json += getAppStr(appStrType::syncResponseReceivedTrue) :
			json += getAppStr(appStrType::syncResponseReceivedFalse);

		aStr = String(year());
		json += getAppStr(appStrType::escapedYear) + aStr + escapedCommaChar;
		aStr = String(month());
		json += getAppStr(appStrType::escapedMonth) + aStr + escapedCommaChar;
		aStr = String(day());
		json += getAppStr(appStrType::escapedDay) + aStr + escapedCommaChar;

		aStr = String(hour());
		json += getAppStr(appStrType::escapedHour) + aStr + escapedCommaChar;
		aStr = String(minute());
		json += getAppStr(appStrType::escapedMinute) + aStr + escapedCommaChar;
		aStr = String(second());
		json += getAppStr(appStrType::escapedSecond) + aStr + escapedCommaChar;


		aStr = String(year(NetworkSvcMngr.ntpManager->lastSync));
		json += getAppStr(appStrType::escapedLastSyncYear) + aStr + escapedCommaChar;
		aStr = String(month(NetworkSvcMngr.ntpManager->lastSync));
		json += getAppStr(appStrType::escapedLastSyncMonth) + aStr + escapedCommaChar;
		aStr = String(day(NetworkSvcMngr.ntpManager->lastSync));
		json += getAppStr(appStrType::escapedLastSyncDay) + aStr + escapedCommaChar;

		aStr = String(hour(NetworkSvcMngr.ntpManager->lastSync));
		json += getAppStr(appStrType::escapedLastSyncHour) + aStr + escapedCommaChar;
		aStr = String(minute(NetworkSvcMngr.ntpManager->lastSync));
		json += getAppStr(appStrType::escapedLastSyncMinute) + aStr + escapedCommaChar;
		aStr = String(second(NetworkSvcMngr.ntpManager->lastSync));
		json += getAppStr(appStrType::escapedLastSyncSecond) + aStr + escapedCommaChar;

		aStr = String(NetworkSvcMngr.ntpManager->lastSyncRetries);
		json += getAppStr(appStrType::escapedLastSyncRetries) + aStr + getAppStr(appStrType::escapedQuote);
	}

	json += getAppStr(appStrType::closeBrace);

	return json;
}

String ICACHE_FLASH_ATTR getRuntimeJsonData() {

	String escapedCommaChar = getAppStr(appStrType::escapedComma);

	String sslReverseProxyForwardedForStr = getAppStr(appStrType::sslReverseProxyForwardedFor);
	String clientIp = webserver->hasHeader(sslReverseProxyForwardedForStr)
			? webserver->header(sslReverseProxyForwardedForStr)
			: webserver->client().remoteIP().toString();

	String json = getAppStr(appStrType::openBrace);

	json += getAppStr(appStrType::escapedFreeHeap) + String(ESP.getFreeHeap()) + escapedCommaChar;
	json += getAppStr(appStrType::escapedFirmwareVersion) + String(TimestampedVersion) + escapedCommaChar;
	json += getAppStr(appStrType::escapedSdkVersion) + String(ESP.getSdkVersion()) + escapedCommaChar;
	json += getAppStr(appStrType::escapedCoreVersion) + String(ESP.getCoreVersion()) + escapedCommaChar;
	json += getAppStr(appStrType::escapedDeviceClass) + String(iD8266Controller) + escapedCommaChar;
	json += getAppStr(appStrType::escapedDeviceClassString) + getAppStr(appStrType::DeviceClass_WiFiController) + escapedCommaChar;
	json += getAppStr(appStrType::escapedDeviceMac) + WiFi.macAddress() + escapedCommaChar;
	json += getAppStr(appStrType::escapedDeviceIp) + WiFi.localIP().toString() + escapedCommaChar;
	json += getAppStr(appStrType::escapedDeviceApIp) + WiFi.softAPIP().toString() + escapedCommaChar;
	json += getAppStr(appStrType::escapedClientIp) + clientIp + escapedCommaChar;
	json += getAppStr(appStrType::escapedBinaryBytes) + String(ESP.getSketchSize()) + escapedCommaChar;
	json += getAppStr(appStrType::escapedFreeBytes) + String(ESP.getFreeSketchSpace()) + escapedCommaChar;
	json += getAppStr(appStrType::escapedDeviceUpTime) + Ntp::getDeviceUptimeString() + getAppStr(appStrType::escapedQuote);

	json += getAppStr(appStrType::closeBrace);

	return json;
}

/*
 * returns a json string containing gpio configuration and state data for a specific gpio pin
 */
String ICACHE_FLASH_ATTR getGpioDataByIdx(uint8_t idx, bool allData) {

	String escapedCommaStr = getAppStr(appStrType::escapedComma);
	String escapedQuoteStr = getAppStr(appStrType::escapedQuote);

	String json = getAppStr(appStrType::openBrace);

	if (idx >= 0 && idx < MAX_DEVICES) {

		if (allData) {
			appConfigData.mqttSystemEnabled
				? json += getAppStr(appStrType::mqttSystemEnabledTrue)
				: json += getAppStr(appStrType::mqttSystemEnabledFalse);

			char ipcstr[16];
			String ipAddrPlaceholder = getAppStr(appStrType::ipAddressTemplateStr);

			sprintf(ipcstr, ipAddrPlaceholder.c_str(),
					appConfigData.mqttServerBrokerIp[0],
					appConfigData.mqttServerBrokerIp[1],
					appConfigData.mqttServerBrokerIp[2],
					appConfigData.mqttServerBrokerIp[3]);

			json += getAppStr(appStrType::escapedMqttServerBrokerIp) + String(ipcstr) + escapedCommaStr;
			json += getAppStr(appStrType::escapedMqttServerBrokerPort) + String(appConfigData.mqttServerBrokerPort) + escapedCommaStr;
			json += getAppStr(appStrType::escapedMqttUsername) + String(appConfigData.mqttUsername) + escapedCommaStr;
			json += getAppStr(appStrType::escapedMqttPassword) + String(appConfigData.mqttPassword) + escapedCommaStr;
			json += getAppStr(appStrType::escapedWebserver) + getWebserverSettings() + getAppStr(appStrType::comma);
		}

		json += getAppStr(appStrType::escapedIdxStr) + String(idx) + escapedCommaStr;
		json += getAppStr(appStrType::escapedName) + String(appConfigData.gpio.digitals[idx].name) + escapedCommaStr;
		json += getAppStr(appStrType::escapedMode) + String(appConfigData.gpio.digitals[idx].pinMode) + escapedCommaStr;
		json += getAppStr(appStrType::escapedDefault) + String(appConfigData.gpio.digitals[idx].defaultValue) + escapedCommaStr;

		peripheralData *peripheral = GPIOMngr.getPeripheralByIdx(idx);
		if (peripheral != NULL) {
			//include the data of the peripheral configured to use this pinIdx
			json += getAppStr(appStrType::escapedPeripheralType) + String(peripheral->type) + escapedCommaStr;

			switch (peripheral->type) {
				case peripheralType::digistatMk2:
					json += getAppStr(appStrType::escapedValueStr) + String(peripheral->base.lastValue) + escapedQuoteStr;
					break;
				case peripheralType::dht22:
					json += getAppStr(appStrType::escapedValueStr) + String(peripheral->lastAnalogValue1, 1) + escapedCommaStr;
					json += getAppStr(appStrType::escapedValue2Str) + String(peripheral->lastAnalogValue2, 1) + escapedQuoteStr;
					break;
				case peripheralType::homeEasySwitch:
					json += getAppStr(appStrType::escapedValueStr) + String(peripheral->base.lastValue) + escapedCommaStr;
					json += getAppStr(appStrType::escapedVirtualDeviceId) + String(peripheral->virtualDeviceId) + escapedQuoteStr;
					break;
				default:
					break;
			}

		} else {
			json += getAppStr(appStrType::escapedValueStr) + String(appConfigData.gpio.digitals[idx].lastValue) + escapedQuoteStr;
		}
	}

	json += getAppStr(appStrType::closeBrace);
	return json;

}

String ICACHE_FLASH_ATTR getMqttData() {

	String json = getAppStr(appStrType::openBrace);

	appConfigData.mqttSystemEnabled
		? json += getAppStr(appStrType::mqttSystemEnabledTrue)
		: json += getAppStr(appStrType::mqttSystemEnabledFalse);

	char ipcstr[16];
	String ipAddrPlaceholder = getAppStr(appStrType::ipAddressTemplateStr);

	sprintf(ipcstr, ipAddrPlaceholder.c_str(),
			appConfigData.mqttServerBrokerIp[0],
			appConfigData.mqttServerBrokerIp[1],
			appConfigData.mqttServerBrokerIp[2],
			appConfigData.mqttServerBrokerIp[3]);

	String escapedCommaChar = getAppStr(appStrType::escapedComma);

	json += getAppStr(appStrType::escapedMqttServerBrokerIp) + String(ipcstr) + escapedCommaChar;
	json += getAppStr(appStrType::escapedMqttServerBrokerPort)	 + String(appConfigData.mqttServerBrokerPort) + escapedCommaChar;
	json += getAppStr(appStrType::escapedMqttUsername) + String(appConfigData.mqttUsername) + escapedCommaChar;
	json += getAppStr(appStrType::escapedMqttPassword) + String(appConfigData.mqttPassword) + escapedCommaChar;
	((appConfigData.mqttSystemEnabled) && (NetworkSvcMngr.mqttConnected()))
		? json += getAppStr(appStrType::mqttConnectedTrue)
		: json += getAppStr(appStrType::mqttConnectedFalse);

	json += getAppStr(appStrType::closeBrace);

	return json;
}

/*
 * returns a json string containing gpio configuration and state data
 */
String ICACHE_FLASH_ATTR getGpioData(bool allData) {

	bool specificIdx = false;
	uint8_t idx = -1;
	if(webserver->hasArg(getAppStr(appStrType::dStr))) {
		String idxStr = webserver->arg(getAppStr(appStrType::dStr));
		char *endptr;
		idx = strtoul(idxStr.c_str(), &endptr, 10);
		if ((*endptr == '\0') && (idx >= 0 && idx < MAX_DEVICES))
			specificIdx = true;
		else
			return EMPTY_STR;
	}
	if (specificIdx)
		//just return the specific gpio requested
		return getGpioDataByIdx(idx, true);

	String escapedCommaChar = getAppStr(appStrType::escapedComma);
	String commaStr = getAppStr(appStrType::comma);
	String quoteStr = getAppStr(appStrType::escapedQuote);

	String json = getAppStr(appStrType::openBrace);

	if (allData) {
		json += getAppStr(appStrType::escapedWebserver) + getWebserverSettings() + commaStr;
	}

	json += getAppStr(appStrType::A0Name) + String(appConfigData.gpio.analogName) + escapedCommaChar;
	json += getAppStr(appStrType::A0Mode) + String(appConfigData.gpio.analogPinMode) + escapedCommaChar;
	json += getAppStr(appStrType::A0Value) + String(appConfigData.gpio.analogRawValue) + escapedCommaChar;
	json += getAppStr(appStrType::A0Voltage) + String(appConfigData.gpio.analogVoltage) + escapedCommaChar;
	json += getAppStr(appStrType::A0Offset) + String(appConfigData.gpio.analogOffset) + escapedCommaChar;
	json += getAppStr(appStrType::A0Multiplier) + String(appConfigData.gpio.analogMultiplier) + escapedCommaChar;
	json += getAppStr(appStrType::A0Unit) + String((int)appConfigData.gpio.analogUnit) + escapedCommaChar;
	json += getAppStr(appStrType::A0UnitStr) + GPIOManager::getUnitOfMeasureStr(appConfigData.gpio.analogUnit) + escapedCommaChar;
	json += getAppStr(appStrType::A0Calculated) + String(appConfigData.gpio.analogCalcVal) + quoteStr;

	if (webserver->hasArg("a")) {
		//just return the analog data
		json += getAppStr(appStrType::closeBrace);
		return json;
	}

	//continue on and return everything
	json += getAppStr(appStrType::gpioArrayWithCommaPrefix);

	for(uint8_t idx = 0; idx < MAX_DEVICES; idx++) {
		json += getGpioDataByIdx(idx, false);
		if(idx != MAX_DEVICES -1)
			json += commaStr;
	}

	json += "]}";

	return json;
}

/*
 returns a json string of combined json types:
 */
String ICACHE_FLASH_ATTR getDiagnosticsData() {

	String commaStr = getAppStr(appStrType::commaAndSpace);

	String json = getAppStr(appStrType::openBrace);
	json += getAppStr(appStrType::runtimeDataStr) + getRuntimeJsonData() + commaStr;
	json += getAppStr(appStrType::netSettingsStr) + getNetSettings() + commaStr;
	json += getAppStr(appStrType::dnsSettingsStr) + getDnsSettings() + commaStr;
	json += getAppStr(appStrType::webserverSettingsStr) + getWebserverSettings() + commaStr;
	json += getAppStr(appStrType::regionalSettingsStr) + getRegionalSettings() + commaStr;
	json += getAppStr(appStrType::mqttDataStr) + getMqttData();
	json += getAppStr(appStrType::closeBrace);
	return json;

}

/*
 returns a json string of combined json types:
 */
String ICACHE_FLASH_ATTR getHomePageData() {

	String json = getAppStr(appStrType::openBrace);
	json += getAppStr(appStrType::runtimeDataStr) + getRuntimeJsonData() + getAppStr(appStrType::commaAndSpace);
	json += getAppStr(appStrType::gpioDataStr) + getGpioData(false);
	json += getAppStr(appStrType::closeBrace);
	return json;

}

/*
 * returns a json string containing power management schedule configuration data for a specific schedule idx position
 */
String ICACHE_FLASH_ATTR getPowerMgmtScheduleDataByIdx(uint8_t idx) {

	String escapedCommaChar = getAppStr(appStrType::escapedComma);
	String json = getAppStr(appStrType::openBrace);

	scheduleData *schedule = &appConfigData.powerMgmt.schedules[idx];
	json += getAppStr(appStrType::escapedIdxStr) + String(idx) + escapedCommaChar;
	json += getAppStr(appStrType::escapedWeekday) + String(schedule->weekday) + escapedCommaChar;
	json += getAppStr(appStrType::escapedHour) + String(schedule->hour) + escapedCommaChar;
	json += getAppStr(appStrType::escapedOffLength) + String(schedule->offLength) + getAppStr(appStrType::escapedQuote);

	json += getAppStr(appStrType::closeBrace);
	return json;
}

/*
 * returns a json string containing power management schedule configuration data
 */
String ICACHE_FLASH_ATTR getPowerMgmtScheduleData(bool wrapped = true) {

	String json;
	String commaChar = getAppStr(appStrType::comma);

	if (wrapped) {
		json += getAppStr(appStrType::openBrace);
	}

	json += getAppStr(appStrType::scheduleArray);

	scheduleData *schedule;

	for(uint8_t idx = 0; idx < MAX_SCHEDULES; idx++) {

		schedule = &appConfigData.powerMgmt.schedules[idx];
		if (schedule->enabled) {
			json += getPowerMgmtScheduleDataByIdx(idx);
			if(idx != MAX_SCHEDULES -1) {
				json += commaChar;
			}
		}

	}

	if (json.endsWith(commaChar)) {
		json = json.substring(0, json.length() - 1);
	}

	json += "]";

	if (wrapped) {
		json += getAppStr(appStrType::closeBrace);
	}

	return json;
}

/*
 * returns a json string containing all power management configuration data
 */
String ICACHE_FLASH_ATTR getPowerMgmtData() {

	String commaChar = getAppStr(appStrType::escapedComma);
	String json = getAppStr(appStrType::openBrace);

	appConfigData.powerMgmt.enabled
		? json += getAppStr(appStrType::escapedEnabledTrue)
		: json += getAppStr(appStrType::escapedEnabledFalse);

	json += getAppStr(appStrType::escapedOnLength) + String(appConfigData.powerMgmt.onLength) + commaChar;

	json += getAppStr(appStrType::escapedOffLength) + String(appConfigData.powerMgmt.offLength) + commaChar;

	json += getPowerMgmtScheduleData(false);

	json += getAppStr(appStrType::closeBrace);
	return json;
}

/*
 * returns a json string containing security related data
 */
String ICACHE_FLASH_ATTR getSecurityData() {

	String json = EMPTY_STR;
	json += getAppStr(appStrType::openBrace);

	appConfigData.ntpEnabled ?
		json += getAppStr(appStrType::ntpEnabledTrue) :
		json += getAppStr(appStrType::ntpEnabledFalse);

	//totp is only when enabled when ntp is enabled and working correctly
	((appConfigData.otp.enabled) &&
	(NetworkSvcMngr.ntpEnabled) &&
	(NetworkSvcMngr.ntpManager->syncResponseReceived)) ?
		json += getAppStr(appStrType::totpEnabledTrue) :
		json += getAppStr(appStrType::totpEnabledFalse);

	json += getAppStr(appStrType::closeBrace);
	return json;
}

/*
 * Returns a json string containing a TOTP QR code uri for a brand new TOTP secret key code.
 * Calling this method creates a new TOTP secret key, stores it in flash and also
 * switches off TOTP in preparation for subsequent activation when the correct token is HTTP POST'd
 * back via submitTotpToken / handleSubmitTotpToken
 */
String ICACHE_FLASH_ATTR getTotpQrCodeUriData() {

	String json = EMPTY_STR;
	json += getAppStr(appStrType::openBrace);

	if ((NetworkSvcMngr.ntpEnabled) &&
		(NetworkSvcMngr.ntpManager->syncResponseReceived) &&
		(ESP8266TOTP::GetNewKey(appConfigData.otp.keyBytes))) {

		appConfigData.otp.enabled = false;

		String argName = getAppStr(appStrType::totpOff);
		if (webserver->hasArg(argName)) {
			FlashAppDataMngr.setAppData();
		}

		String qrCodeUri = ESP8266TOTP::GetQrCodeImageUri(appConfigData.otp.keyBytes, appConfigData.hostName, "Intelligent%20Devices");

		json += getAppStr(appStrType::totpQrCodeUri) + qrCodeUri + getAppStr(appStrType::escapedQuote);
	}

	json += getAppStr(appStrType::closeBrace);
	return json;

}

/*
 * returns a json string containing peripheral configuration and state data for a specific deviceIdx
 */
String ICACHE_FLASH_ATTR getPeripheralDataByIdx(uint8_t deviceIdx) {

	String escapedCommaChar = getAppStr(appStrType::escapedComma);
	String quote = getAppStr(appStrType::escapedQuote);

	String json = getAppStr(appStrType::openBrace);

	if (deviceIdx >= 0 && deviceIdx < MAX_DEVICES) {

		peripheralData *peripheral = &appConfigData.device.peripherals[deviceIdx];

		json += getAppStr(appStrType::escapedIdxStr) + String(deviceIdx) + escapedCommaChar;
		json += getAppStr(appStrType::escapedPeripheralType) + String(peripheral->type) + escapedCommaChar;
		json += getAppStr(appStrType::escapedPeripheralPin) + String(peripheral->pinIdx) + escapedCommaChar;
		json += getAppStr(appStrType::escapedVirtualDeviceId) + String(peripheral->virtualDeviceId) + escapedCommaChar;
		json += getAppStr(appStrType::escapedName) + String(peripheral->base.name) + quote;

	}

	json += getAppStr(appStrType::closeBrace);
	return json;

}

/*
 * returns a json string containing peripheral configuration data
 */
String ICACHE_FLASH_ATTR getPeripheralsData() {

	String json;
	String commaChar = getAppStr(appStrType::comma);

	json += getAppStr(appStrType::openBrace);
	json += getAppStr(appStrType::peripheralArray);

	peripheralData *peripheral;

	for(uint8_t deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {

		peripheral = &appConfigData.device.peripherals[deviceIdx];
		if (peripheral->type != peripheralType::unspecified) {
			json += getPeripheralDataByIdx(deviceIdx);
			if(deviceIdx != MAX_DEVICES -1) {
				json += commaChar;
			}
		}
	}

	if (json.endsWith(commaChar)) {
		json = json.substring(0, json.length() - 1);
	}

	json += "]";

	json += getAppStr(appStrType::closeBrace);

	return json;
}

String ICACHE_FLASH_ATTR GetJsonData(jsonResponseType type) {

	String json;

	switch(type)
	{
		case networkSettings:
			json = getNetSettings();
			break;
		case dnsSettings:
			json = getDnsSettings();
			break;
		case webserverSettings:
			json = getWebserverSettings();
			break;
		case regionalSettings:
			json = getRegionalSettings();
			break;
		case diagnosticsData:
			json = getDiagnosticsData();
			break;
		case mqttData:
			json = getMqttData();
			break;
		case gpioData:
			json = getGpioData(true);
			break;
		case homePageData:
			json = getHomePageData();
			break;
		case powerMgmtData:
			json = getPowerMgmtData();
			break;
		case powerMgmtScheduleData:
			json = getPowerMgmtScheduleData();
			break;
		case securityData:
			json = getSecurityData();
			break;
		case totpQrCodeUriData:
			json = getTotpQrCodeUriData();
			break;
		case peripheralsData:
			json = getPeripheralsData();
			break;
	}

	return json;
}

bool ICACHE_FLASH_ATTR handleGetJsonResponse(jsonResponseType type) {

	if (type == jsonResponseType::securityData || isAuthenticated()) {

		String json = GetJsonData(type);
		includeNoCacheHeaders();
		webserver->send(200, getAppStr(appStrType::jsonContentType), json);
		return true;

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));
		return false;

	}
}

void ICACHE_FLASH_ATTR saveIpAddress(uint8_t *byteArr, String &ipaddrStr) {

	IPAddress ipaddr;
	bool parseSuccess;

	parseSuccess = ipaddr.fromString(ipaddrStr);
	if (parseSuccess) {
		for (int i = 0; i < 4; i++) {
			byteArr[i] = ipaddr[i];
		}
	}
}

/*
 network settings postback save handler
 */
void ICACHE_FLASH_ATTR handleSaveNetSettings() {

	if (isAuthenticated()) {

		String wifiModeStr = webserver->arg(getAppStr(appStrType::wifiMode));
		WiFiMode wifiMode = (WiFiMode) strtoul(wifiModeStr.c_str(), NULL, 10);

		if (wifiMode != WIFI_OFF) {

			appConfigData.wifiMode = wifiMode;

			/*AP params*/
			String apSSID = webserver->arg(getAppStr(appStrType::apSSID));
			String apPwd = webserver->arg(getAppStr(appStrType::apPwd));

			String apChannelStr = webserver->arg(getAppStr(appStrType::apChannel));
			byte apChannel = (byte) strtoul(apChannelStr.c_str(), NULL, 10);

			/*STA params*/
			String netApSSID = webserver->arg(getAppStr(appStrType::netApSSID));
			String netApPwd = webserver->arg(getAppStr(appStrType::netApPwd));

			String networkApIpModeStr = webserver->arg(getAppStr(appStrType::netApIpMode));
			ipMode networkApIpMode = (ipMode) strtoul(networkApIpModeStr.c_str(), NULL, 10);

			String netApStaticIpStr = webserver->arg(getAppStr(appStrType::netApStaticIp));
			String netApSubnetStr = webserver->arg(getAppStr(appStrType::netApSubnet));
			String netApGatewayIpStr = webserver->arg(getAppStr(appStrType::netApGatewayIp));
			String netApDnsIpStr = webserver->arg(getAppStr(appStrType::netApDnsIp));
			String netApDns2IpStr = webserver->arg(getAppStr(appStrType::netApDns2Ip));

			if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA) {
				//validate and set AP params
				if (apSSID.length() > 0)
					strncpy(appConfigData.deviceApSSID, apSSID.c_str(), STRMAX);
				if (apPwd.length() >= 8)
					strncpy(appConfigData.deviceApPwd, apPwd.c_str(), STRMAX);
				if (apChannel > 0 && apChannel < 14)
					appConfigData.deviceApChannel = apChannel;
			}

			if (wifiMode == WIFI_STA || wifiMode == WIFI_AP_STA) {

				//validate and set STA params
				if (netApSSID.length() > 0)
					strncpy(appConfigData.networkApSSID, netApSSID.c_str(), STRMAX);
				if (netApPwd.length() >= 8)
					strncpy(appConfigData.networkApPwd, netApPwd.c_str(), STRMAX);

				if (networkApIpMode == 0 || networkApIpMode == 1) {
					appConfigData.networkApIpMode = networkApIpMode;

					if (networkApIpMode == 1) {
						//static IPs
						saveIpAddress(appConfigData.networkApStaticIp, netApStaticIpStr);
						saveIpAddress(appConfigData.networkApSubnet, netApSubnetStr);
						saveIpAddress(appConfigData.networkApGatewayIp, netApGatewayIpStr);
						saveIpAddress(appConfigData.networkApDnsIp, netApDnsIpStr);
						saveIpAddress(appConfigData.networkApDns2Ip, netApDns2IpStr);
					}
				}

				if (!appConfigData.ntpEnabled) {
					//auto ntp client switch on
					appConfigData.ntpEnabled = true;
				}
			}

			persistConfig();
		}

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 dns settings postback save handler
 */
void ICACHE_FLASH_ATTR handleSaveDnsSettings() {

	if (isAuthenticated()) {

		String on = getAppStr(appStrType::onStr);

		String hostNameStr = webserver->arg(getAppStr(appStrType::hostName));
		if (hostNameStr.length() >= 8)
			strncpy(appConfigData.hostName, hostNameStr.c_str(), STRMAX);

		if(webserver->hasArg(getAppStr(appStrType::mdnsEnabled))) {
			String mdnsEnabledStr = webserver->arg(getAppStr(appStrType::mdnsEnabled));
			appConfigData.mdnsEnabled = (mdnsEnabledStr.equals(on));
		} else
			appConfigData.mdnsEnabled = false;

		if(webserver->hasArg(getAppStr(appStrType::dnsEnabled))) {
			String dnsEnabledStr = webserver->arg(getAppStr(appStrType::dnsEnabled));
			appConfigData.dnsEnabled = (dnsEnabledStr.equals(on));
		} else
			appConfigData.dnsEnabled = false;

		String dnsTTLStr = webserver->arg(getAppStr(appStrType::dnsTTL));
		uint32_t intval = strtoul(dnsTTLStr.c_str(), NULL, 10);
		if (intval >= 1 && intval <= 65535)
			appConfigData.dnsTTL = intval;

		String dnsPortStr = webserver->arg(getAppStr(appStrType::dnsPort));
		uint16_t wordval = strtoul(dnsPortStr.c_str(), NULL, 10);
		if (wordval >= 1 && wordval <= 65535)
			appConfigData.dnsPort = wordval;

		if(webserver->hasArg(getAppStr(appStrType::dnsCatchAll))) {
			String dnsCatchAllStr = webserver->arg(getAppStr(appStrType::dnsCatchAll));
			appConfigData.dnsCatchAll = (dnsCatchAllStr.equals(on));
		} else
			appConfigData.dnsCatchAll = false;

		persistConfig();

	} else {
		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));
	}
}

/*
 * soft reboot handler
 */
void ICACHE_FLASH_ATTR handleReboot() {
	if (isAuthenticated()) {
		NetworkSvcMngr.powerManager->isDisabled = true;
		returnResource(getAppStr(appStrType::updateDoneMinHtm));
		rebootTimeStamp = millis() + 1000;

	} else {
		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));
	}
}

/*
 * factory reset handler
 */
void ICACHE_FLASH_ATTR handleReset() {
	if (isAuthenticated()) {
		NetworkSvcMngr.powerManager->isDisabled = true;
		FlashAppDataMngr.initFlash(true, false);
		returnResource(getAppStr(appStrType::updateDoneMinHtm));
		rebootTimeStamp = millis() + 1000;

	} else {
		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));
	}
}



/*
 webserver settings postback save handler
 */
void ICACHE_FLASH_ATTR handleSaveWebserverSettings() {

	if (isAuthenticated()) {

		uint16_t wordval;
		String argStr;

		argStr = webserver->arg(getAppStr(appStrType::webserverPort));
		wordval = strtoul(argStr.c_str(), NULL, 10);
		if (wordval >= 1 && wordval <= 65535)
			appConfigData.webserverPort = wordval;

		argStr = webserver->arg(getAppStr(appStrType::websocketServerPort));
		wordval = strtoul(argStr.c_str(), NULL, 10);
		if (wordval >= 1 && wordval <= 65535)
			appConfigData.websocketServerPort = wordval;

		if(webserver->hasArg(getAppStr(appStrType::includeServerHeader))) {
			argStr = webserver->arg(getAppStr(appStrType::includeServerHeader));
			appConfigData.includeServerHeader = (argStr.equals(getAppStr(appStrType::onStr)));
		} else
			appConfigData.includeServerHeader = false;

		argStr = webserver->arg(getAppStr(appStrType::serverHeader));
		if (argStr.length() >= 8)
			strncpy(appConfigData.serverHeader, argStr.c_str(), STRMAX);

		persistConfig();

	} else {
		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));
	}
}


/*
 regional settings postback save handler
 */
void ICACHE_FLASH_ATTR handleSaveRegionalSettings() {

	if (isAuthenticated()) {

		if(webserver->hasArg(getAppStr(appStrType::ntpEnabled))) {
			String ntpEnabledStr = webserver->arg(getAppStr(appStrType::ntpEnabled));
			appConfigData.ntpEnabled = (ntpEnabledStr.equals(getAppStr(appStrType::onStr)));
		} else
			appConfigData.ntpEnabled = false;

		String ntpServerStr = webserver->arg(getAppStr(appStrType::ntpServer));
		uint8_t ubyteval = strtoul(ntpServerStr.c_str(), NULL, 10);
		if (ubyteval >= 0 && ubyteval <= 5)
			appConfigData.ntpServer = ubyteval;

		String ntpTimeZoneStr = webserver->arg(getAppStr(appStrType::ntpTimeZone));
		int8_t byteval = strtol(ntpTimeZoneStr.c_str(), NULL, 10);
		if (byteval >= -12 && byteval <= 12)
			appConfigData.ntpTimeZone = byteval;

		String ntpSyncIntervalStr = webserver->arg(getAppStr(appStrType::ntpSyncInterval));
		time_t timeVal = strtol(ntpSyncIntervalStr.c_str(), NULL, 10);
		if (timeVal >= 3600)
			appConfigData.ntpSyncInterval = timeVal;

		String ntpLocaleStr = webserver->arg(getAppStr(appStrType::ntpLocale));
		if (ntpLocaleStr.length() >= 5 && ntpLocaleStr.length() <= 6)
			strncpy(appConfigData.ntpLocale, ntpLocaleStr.c_str(), STRLOCALE);

		persistConfig();

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 * gpio settings postback save handler for a single GPIO pin
 */
void ICACHE_FLASH_ATTR saveGpioDataByIdx(uint8_t idx) {

	if (idx >= 0 && idx < MAX_DEVICES) {

		String argName;
		String argStr;
		uint8_t ubyteval;
		char *endptr;
		String pre = getAppStr(appStrType::dStr) + String(idx);

		argName = pre + getAppStr(appStrType::PinMode);
		if(webserver->hasArg(argName)) {
			argStr = webserver->arg(argName.c_str());
			ubyteval = strtoul(argStr.c_str(), &endptr, 10);
			if ((*endptr == '\0') && (ubyteval >= 0 && ubyteval <= 4)) {
				digitalMode pinMode = (digitalMode)ubyteval;
				if (pinMode != digitalMode::digitalNotInUse) {
					//this GPIO is being specifically configured in some way so drop any
					//peripherals configured for this pin
					GPIOMngr.removePeripheralsByPinIdx(idx);
				}
				appConfigData.gpio.digitals[idx].pinMode = pinMode;
			}
		}

		argName = pre + getAppStr(appStrType::Name);
		if(webserver->hasArg(argName)) {
			argStr = webserver->arg(argName.c_str());
			if (argStr.length() > 0)
				strncpy(appConfigData.gpio.digitals[idx].name, argStr.c_str(), STRMAX);
		}

		argName = pre + getAppStr(appStrType::Default);
		if(webserver->hasArg(argName)) {
			argStr = webserver->arg(argName.c_str());
			ubyteval = strtoul(argStr.c_str(), &endptr, 10);
			if ((*endptr == '\0') && (ubyteval >= 0 && ubyteval <= 255)) {

				switch (appConfigData.gpio.digitals[idx].pinMode)
				{
				case digitalOutput:
					if ((ubyteval == 0) || (ubyteval == 1)) {
						appConfigData.gpio.digitals[idx].defaultValue = ubyteval;
					} else {
						appConfigData.gpio.digitals[idx].defaultValue = 0;
					}
					break;
				default:
					appConfigData.gpio.digitals[idx].defaultValue = ubyteval;
					break;
				}
			}
		}
	}
}

/*
 * mqtt settings postback save handler
 */
void ICACHE_FLASH_ATTR handleSaveMqttSettings() {

	if (isAuthenticated()) {

		String argStr;
		uint16_t ushortval;
		char *endptr;

		String on = getAppStr(appStrType::onStr);

		if(webserver->hasArg(getAppStr(appStrType::mqttSystemEnabled))) {
			argStr = webserver->arg(getAppStr(appStrType::mqttSystemEnabled));
			appConfigData.mqttSystemEnabled = (argStr.equals(on));
		} else
			appConfigData.mqttSystemEnabled = false;

		if (appConfigData.mqttSystemEnabled) {

			argStr = webserver->arg(getAppStr(appStrType::mqttServerBrokerIp));
			saveIpAddress(appConfigData.mqttServerBrokerIp, argStr);

			argStr = webserver->arg(getAppStr(appStrType::mqttServerBrokerPort));
			ushortval = strtoul(argStr.c_str(), &endptr, 10);
			if ((*endptr == '\0') && (ushortval >= 1 && ushortval <= 65535))
				appConfigData.mqttServerBrokerPort = ushortval;

			argStr = webserver->arg(getAppStr(appStrType::mqttUsername));
			if (argStr.length() > 0)
				strncpy(appConfigData.mqttUsername, argStr.c_str(), STRMAX);

			argStr = webserver->arg(getAppStr(appStrType::mqttPassword));
			if (argStr.length() > 0)
				strncpy(appConfigData.mqttPassword, argStr.c_str(), STRMAX);
		}

		persistConfig();

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}

}

/*
 * gpio settings postback save handler
 */
void ICACHE_FLASH_ATTR handleSaveGpioSettings() {

	if (isAuthenticated()) {

		String argStr;
		uint8_t ubyteval;
		uint16_t ushortval;
		double doubleVal;
		char *endptr;

		for(uint8_t idx = 0; idx < MAX_DEVICES; idx++) {
			saveGpioDataByIdx(idx);
		}

		if (webserver->hasArg(getAppStr(appStrType::a0PinMode))) {
			argStr = webserver->arg(getAppStr(appStrType::a0PinMode));
			ubyteval = strtoul(argStr.c_str(), &endptr, 10);
			if ((*endptr == '\0') && (ubyteval >= 0 && ubyteval <= 1)) {
				appConfigData.gpio.analogPinMode = (analogMode)ubyteval;
			}
		}

		if (webserver->hasArg(getAppStr(appStrType::a0Name))) {
			argStr = webserver->arg(getAppStr(appStrType::a0Name));
			if (argStr.length() > 0)
				strncpy(appConfigData.gpio.analogName, argStr.c_str(), STRMAX);
		}

		if(webserver->hasArg(getAppStr(appStrType::a0Offset))) {
			argStr = webserver->arg(getAppStr(appStrType::a0Offset));
			doubleVal = strtod(argStr.c_str(), &endptr);
			if (*endptr == '\0')
				appConfigData.gpio.analogOffset = doubleVal;
		}

		if(webserver->hasArg(getAppStr(appStrType::a0Multiplier))) {
			argStr = webserver->arg(getAppStr(appStrType::a0Multiplier));
			doubleVal = strtod(argStr.c_str(), &endptr);
			if (*endptr == '\0')
				appConfigData.gpio.analogMultiplier = doubleVal;
		}

		if(webserver->hasArg(getAppStr(appStrType::a0Unit))) {
			argStr = webserver->arg(getAppStr(appStrType::a0Unit));
			ushortval = strtoul(argStr.c_str(), &endptr, 10);
			if ((*endptr == '\0') && (ushortval >= 0 && ushortval <= unitOfMeasure::unitOfMeasureMax))
				appConfigData.gpio.analogUnit = (unitOfMeasure)ushortval;
		}

		persistConfig();

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 * GPIO digital write GET handler
 */
bool ICACHE_FLASH_ATTR handleDigitalWrite() {

	if (isAuthenticated() && (webserver->hasArg(getAppStr(appStrType::dStr))) && (webserver->hasArg(getAppStr(appStrType::valueStr)))) {

		char *pinIdxEndPtr;
		char *valueEndPtr;
		uint8_t pinIdx = strtoul(webserver->arg(getAppStr(appStrType::dStr)).c_str(), &pinIdxEndPtr, 10);
		uint8_t digitalValue = strtoul(webserver->arg(getAppStr(appStrType::valueStr)).c_str(), &valueEndPtr, 10);

		bool result = GPIOMngr.gpioDigitalWrite(pinIdx, digitalValue);
		webserver->send(200, getAppStr(appStrType::jsonContentType), result ? getAppStr(appStrType::jsonResultOk) : getAppStr(appStrType::jsonResultFailed));
		return result;

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
	return false;
}

/*
 * GPIO analog write GET handler
 */
bool ICACHE_FLASH_ATTR handleAnalogWrite() {

	if (isAuthenticated() && (webserver->hasArg(getAppStr(appStrType::dStr))) && (webserver->hasArg(getAppStr(appStrType::valueStr)))) {

		char *pinIdxEndPtr;
		char *valueEndPtr;
		uint8_t pinIdx = strtoul(webserver->arg(getAppStr(appStrType::dStr)).c_str(), &pinIdxEndPtr, 10);
		uint8_t analogValue = strtoul(webserver->arg(getAppStr(appStrType::valueStr)).c_str(), &valueEndPtr, 10);

		bool result = GPIOMngr.gpioAnalogWrite(pinIdx, analogValue);
		webserver->send(200, getAppStr(appStrType::jsonContentType), result ? "{\"result\":\"OK\",\"idx\":\"" + String(pinIdx) + "\",\"value\":\"" + String(analogValue) + "\"}" : getAppStr(appStrType::jsonResultFailed));
		return result;

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
	return false;
}

/*
 * peripheral write GET handler
 */
bool ICACHE_FLASH_ATTR handlePeripheralWrite() {

	if (isAuthenticated() &&
        (webserver->hasArg(getAppStr(appStrType::dStr))) &&
        (webserver->hasArg(getAppStr(appStrType::pStr))) &&
        (webserver->hasArg(getAppStr(appStrType::vidStr))) &&
        (webserver->hasArg(getAppStr(appStrType::valueStr)))) {

		uint8_t deviceIdx = strtoul(webserver->arg(getAppStr(appStrType::dStr)).c_str(), NULL, 10);
		peripheralType ptype = (peripheralType)strtoul(webserver->arg(getAppStr(appStrType::pStr)).c_str(), NULL, 10);
		uint8_t virtualDeviceId = strtoul(webserver->arg(getAppStr(appStrType::vidStr)).c_str(), NULL, 10);
		uint8_t digitalValue = strtoul(webserver->arg(getAppStr(appStrType::valueStr)).c_str(), NULL, 10);

		bool result = GPIOMngr.peripheralWrite(deviceIdx, ptype, virtualDeviceId, digitalValue);
		webserver->send(200, getAppStr(appStrType::jsonContentType), result ? getAppStr(appStrType::jsonResultOk) : getAppStr(appStrType::jsonResultFailed));
		return result;

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
	return false;
}

/*
 * power management settings AJAX postback save handler
 */
void ICACHE_FLASH_ATTR handleSavePowerMgmtSettings() {

	if (isAuthenticated()) {

		String argStr;
		uint8_t ubyteval;
		char *endptr;

		String on = getAppStr(appStrType::onStr);

		if(webserver->hasArg(getAppStr(appStrType::enabled))) {
			argStr = webserver->arg(getAppStr(appStrType::enabled));
			appConfigData.powerMgmt.enabled = (argStr.equals(on));
		} else
			appConfigData.powerMgmt.enabled = false;

		argStr = webserver->arg(getAppStr(appStrType::onLength));
		ubyteval = strtoul(argStr.c_str(), &endptr, 10);
		if (*endptr == '\0' && ubyteval >=0 && ubyteval <= powerOnLength::maxPowerOnLength) {
			appConfigData.powerMgmt.onLength = (powerOnLength)ubyteval;
		}

		argStr = webserver->arg(getAppStr(appStrType::offLength));
		ubyteval = strtoul(argStr.c_str(), &endptr, 10);
		if (*endptr == '\0' && ubyteval >=0 && ubyteval <= powerDownLength::maxPowerDownLength) {
			appConfigData.powerMgmt.offLength = (powerDownLength)ubyteval;
		}

		NetworkSvcMngr.powerManager->delaySleep();

		persistConfig(false);

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 * power management add schedule AJAX postback save handler
 */
void ICACHE_FLASH_ATTR handleAddPowerMgmtSchedule() {

	if (isAuthenticated()) {

		String argStr;
		uint8_t weekday;
		uint8_t hour;
		uint8_t ubyteVal;
		powerDownLength offLength;
		char *endptr;

		bool valid = true;

		argStr = webserver->arg(getAppStr(appStrType::offLength));
		ubyteVal = strtoul(argStr.c_str(), &endptr, 10);
		if (*endptr == '\0' && ubyteVal >=0 && ubyteVal <= powerDownLength::maxPowerDownLength) {
			offLength = (powerDownLength)ubyteVal;
		} else {
			valid = false;
		}

		if (valid) {
			argStr = webserver->arg(getAppStr(appStrType::hourStr));
			ubyteVal = strtoul(argStr.c_str(), &endptr, 10);
			if (*endptr == '\0') {
				hour = ubyteVal;
			} else {
				valid = false;
			}
		}

		if (valid) {
			argStr = webserver->arg(getAppStr(appStrType::weekdayStr));
			ubyteVal = strtoul(argStr.c_str(), &endptr, 10);
			if (*endptr == '\0') {
				weekday = ubyteVal;
			} else {
				valid = false;
			}
		}

		if (valid) {
			if (NetworkSvcMngr.powerManager->addSchedule(weekday, hour, offLength)) {
				FlashAppDataMngr.setAppData();
			}
		}

		handleGetJsonResponse(powerMgmtScheduleData);


	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 * power management remove schedule AJAX postback save handler
 */
void ICACHE_FLASH_ATTR handleRemovePowerMgmtSchedule() {

	if (isAuthenticated()) {

		String argStr;
		uint8_t scheduleIdx;
		char *endptr;

		argStr = webserver->arg(getAppStr(appStrType::idxStr));
		scheduleIdx = strtoul(argStr.c_str(), &endptr, 10);
		if (*endptr == '\0') {
			if (NetworkSvcMngr.powerManager->removeSchedule(scheduleIdx)) {
				FlashAppDataMngr.setAppData();
			}
		}

		handleGetJsonResponse(powerMgmtScheduleData);


	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 * password change postback handler
 */
void ICACHE_FLASH_ATTR handleChangePassword() {

	if (isAuthenticated()) {

		String pwd = webserver->arg(getAppStr(appStrType::pwd));
		String confirmPwd = webserver->arg(getAppStr(appStrType::confirmPwd));

		if ((pwd.equals(confirmPwd)) && (Complexify::IsPasswordValid(pwd))) {

			strncpy(appConfigData.adminPwd, pwd.c_str(), STRMAX);

			bool hasDefaultApPwd = (strcmp(DEFAULT_PWD, appConfigData.deviceApPwd) == 0);
			if (hasDefaultApPwd) {
				//if the deviceApPwd is weak, change it to this new complex password too
				strncpy(appConfigData.deviceApPwd, pwd.c_str(), STRMAX);
			}

			FlashAppDataMngr.setAppData();
			handleLogoff();

		} else {

			redirect(getAppStr(appStrType::passwordStr), getAppStr(appStrType::htmlContentType));

		}

	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 * totp token postback handler
 */
void ICACHE_FLASH_ATTR handleSubmitTotpToken() {

	if (isAuthenticated()) {

		if ((NetworkSvcMngr.ntpEnabled) &&
			(NetworkSvcMngr.ntpManager->syncResponseReceived)) {

			if (isTotpTokenValid()) {
				appConfigData.otp.enabled = true;
				FlashAppDataMngr.setAppData();
			}

		}

		handleGetJsonResponse(securityData);


	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}

}

/*
 * add peripheral AJAX postback save handler
 */
void ICACHE_FLASH_ATTR handleAddPeripheral() {

	if (isAuthenticated()) {

		String argStr;
		peripheralType pType;
		String peripheralName;
		uint8_t pinIdx;
		uint8_t defaultValue;
		uint8_t virtualDeviceId;
		uint8_t ubyteVal;

		char *endptr;

		bool valid = true;

		argStr = webserver->arg(getAppStr(appStrType::peripheralTypeStr));
		ubyteVal = strtoul(argStr.c_str(), &endptr, 10);
		if (*endptr == '\0' && ubyteVal >=0 && ubyteVal <= peripheralType::maxPeripheralType) {
			pType = (peripheralType)ubyteVal;
		} else {
			valid = false;
		}

		if (valid) {
			argStr = webserver->arg(getAppStr(appStrType::Name));
			if (argStr.length() > 0) {
				peripheralName = String(argStr);
			} else {
				valid = false;
			}
		}

		if (valid) {
			argStr = webserver->arg(getAppStr(appStrType::idxStr));
			ubyteVal = strtoul(argStr.c_str(), &endptr, 10);
			if (*endptr == '\0') {
				pinIdx = ubyteVal;
			} else {
				valid = false;
			}
		}

		if(valid) {
			argStr = webserver->arg(getAppStr(appStrType::Default));
			ubyteVal = strtoul(argStr.c_str(), &endptr, 10);
			if ((*endptr == '\0') && (ubyteVal >= 0 && ubyteVal <= 255)) {
				defaultValue = ubyteVal;
			} else {
				valid = false;
			}
		}

		if(valid) {
			argStr = webserver->arg(getAppStr(appStrType::virtualDeviceIdStr));
			ubyteVal = strtoul(argStr.c_str(), &endptr, 10);
			if ((*endptr == '\0') && (ubyteVal >= 0 && ubyteVal <= 255)) {
				virtualDeviceId = ubyteVal;
			} else {
				valid = false;
			}
		}

		if (valid) {
			if (GPIOMngr.addPeripheral(pType, peripheralName, pinIdx, defaultValue, virtualDeviceId)) {
				FlashAppDataMngr.setAppData();
			}
		}

		handleGetJsonResponse(peripheralsData);


	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 * remove peripheral AJAX postback save handler
 */
void ICACHE_FLASH_ATTR handleRemovePeripheral() {

	if (isAuthenticated()) {

		String argStr;
		uint8_t deviceIdx;
		char *endptr;

		argStr = webserver->arg(getAppStr(appStrType::idxStr));
		deviceIdx = strtoul(argStr.c_str(), &endptr, 10);
		if (*endptr == '\0') {
			if (GPIOMngr.removePeripheral(deviceIdx)) {
				FlashAppDataMngr.setAppData();
			}
		}

		handleGetJsonResponse(peripheralsData);


	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

void ICACHE_FLASH_ATTR handleFileList() {

	if (isAuthenticated()) {

		if(!webserver->hasArg(getAppStr(appStrType::dirStr))) {
			webserver->send(500, getAppStr(appStrType::textContentType), getAppStr(appStrType::badArgs));
			return;
		}

		String dirChar = getAppStr(appStrType::dirStr);
		String fileChar = getAppStr(appStrType::fileStr);

		String path = webserver->arg(dirChar);
		Dir dir = SPIFFS.openDir(path);
		path = String();

		String output = "[";
		while(dir.next()) {
			File entry = dir.openFile("r");
			if (output != "[") output += ',';
			bool isDir = false;
			output += "{\"type\":\"";
			output += (isDir) ? dirChar : fileChar;
			output += "\",\"name\":\"";
			output += String(entry.name()).substring(1);
			output += "\"}";
			entry.close();
		}

		output += "]";

		includeNoCacheHeaders();
		webserver->send(200, getAppStr(appStrType::textJsonContentType), output);
	} else {
		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));
	}

}

void ICACHE_FLASH_ATTR handleFileCreate() {

	if (isAuthenticated()) {
		if(webserver->args() == 0)
		    return webserver->send(500, getAppStr(appStrType::textContentType), getAppStr(appStrType::badArgs));

		  String path = webserver->arg(0);

		  if(path == getAppStr(appStrType::FORWARD_SLASH))
			  return webserver->send(500, getAppStr(appStrType::textContentType), getAppStr(appStrType::badPath));

		  if(SPIFFS.exists(path))
			  return webserver->send(500, getAppStr(appStrType::textContentType), getAppStr(appStrType::fileExists));

		  File file = SPIFFS.open(path, "w");
		  if(file)
			  file.close();
		  else
			  return webserver->send(500, getAppStr(appStrType::textContentType), getAppStr(appStrType::createFailed));

		  webserver->send(200, getAppStr(appStrType::textContentType), EMPTY_STR);
		  path = String();
	} else {
		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));
	}
}

void ICACHE_FLASH_ATTR handleFileDelete() {

	if (isAuthenticated()) {
		if(webserver->args() == 0)
			  return webserver->send(500, getAppStr(appStrType::textContentType), getAppStr(appStrType::badArgs));

		  String path = webserver->arg(0);

		  if(path == getAppStr(appStrType::FORWARD_SLASH))
		    return webserver->send(500, getAppStr(appStrType::textContentType), getAppStr(appStrType::badPath));

		  if(!SPIFFS.exists(path))
		    return webserver->send(404, getAppStr(appStrType::textContentType), "FileNotFound");

		  SPIFFS.remove(path);
		  webserver->send(200, getAppStr(appStrType::textContentType), EMPTY_STR);
		  path = String();
	} else {
		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));
	}
}

void ICACHE_FLASH_ATTR handleFileUpload() {

	if (isAuthenticated()) {

		if(webserver->uri() != getAppStr(appStrType::edit))
			return;

		HTTPUpload& upload = webserver->upload();

		if (upload.status == UPLOAD_FILE_START) {

			String filename = upload.filename;
			if(!filename.startsWith(getAppStr(appStrType::FORWARD_SLASH)))
				filename = getAppStr(appStrType::FORWARD_SLASH) + filename;

			if (SPIFFS.exists(filename))
				SPIFFS.remove(filename);

			fsUploadFile = SPIFFS.open(filename, "w");
			filename = String();

		} else
			if(upload.status == UPLOAD_FILE_WRITE) {

				if(fsUploadFile)
					fsUploadFile.write(upload.buf, upload.currentSize);
			} else
				if(upload.status == UPLOAD_FILE_END) {

					if(fsUploadFile)
						fsUploadFile.close();
			  }
	} else {

		webserver->send(403, getAppStr(appStrType::textContentType), getAppStr(appStrType::accessDenied));

	}
}

/*
 * returns a redirect to the given target
 */
void ICACHE_FLASH_ATTR redirect(const String &target, const String &contentType) {

	includeNoCacheHeaders();
	webserver->sendHeader(getAppStr(appStrType::locationStr), schemeAndDomain() + target);
	webserver->send(301, contentType, EMPTY_STR);

}


/*
 * instantiates the web server, configures all supported routes and starts the server
 */
bool ICACHE_FLASH_ATTR configureWebServices() {

	/*
	 * the handler responsible for css and image GET request responses
	 */
	class StaticResourceHandler: public RequestHandler {

		bool canHandle(HTTPMethod requestMethod, String requestUri) override {

			return ((requestMethod == HTTP_GET)
					&& (requestUri.endsWith(getAppStr(appStrType::cssExtension))
							|| requestUri.endsWith(getAppStr(appStrType::jsExtension))
							|| requestUri.endsWith(getAppStr(appStrType::pngExtension))));

		}

		bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) override {

			if (!canHandle(requestMethod, requestUri)) {
				return false;
			}

			return returnResource(requestUri, false); //css, js and png resources are auth free
		}

	};

	/*
	 * the handler responsible for html GET request responses
	 */
	class HtmlGetResourceHandler: public RequestHandler {

		bool canHandle(HTTPMethod requestMethod, String requestUri) override {

			return
			(
				(requestMethod == HTTP_GET) &&
				(
					(requestUri.indexOf(getAppStr(appStrType::idHtmlExtension)) > 0) ||
					(requestUri.equals(getAppStr(appStrType::FORWARD_SLASH)))
				)
			);

		}

		bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) override {

			if (!canHandle(requestMethod, requestUri)) {
				return false;
			}

			bool requiresAuth = !requestUri.equals(getAppStr(appStrType::loginStr));

			if (requestUri.equals(getAppStr(appStrType::FORWARD_SLASH)))
			{
				requestUri = getAppStr(appStrType::indexStr);
			}
			else
			{
				requestUri.replace(getAppStr(appStrType::idHtmlExtension), getAppStr(appStrType::resourcePostFix));
			}

			return returnResource(requestUri, requiresAuth);
		}

	};

	/*
	 * the handler responsible for ajax JSON request responses
	 */
	class AjaxGetResourceHandler: public RequestHandler {

		bool canHandle(HTTPMethod requestMethod, String requestUri) override {

			return
			(
				(requestMethod == HTTP_GET) &&
				(requestUri.startsWith(getAppStr(appStrType::getJsonPrefix)))
			);

		}

		bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) override {

			if (!canHandle(requestMethod, requestUri)) {
				return false;
			}

			if (requestUri.equals(getAppStr(appStrType::getSecurityDataStr))) {
				return handleGetJsonResponse(securityData);
			}
			if (requestUri.equals(getAppStr(appStrType::getHomePageDataStr))) {
				return handleGetJsonResponse(homePageData);
			}
			if (requestUri.equals(getAppStr(appStrType::getNetSettingsStr))) {
				return handleGetJsonResponse(networkSettings);
			}
			if (requestUri.equals(getAppStr(appStrType::getDnsSettingsStr))) {
				return handleGetJsonResponse(dnsSettings);
			}
			if (requestUri.equals(getAppStr(appStrType::getWebserverSettingsStr))) {
				return handleGetJsonResponse(webserverSettings);
			}
			if (requestUri.equals(getAppStr(appStrType::getRegionalSettingsStr))) {
				return handleGetJsonResponse(regionalSettings);
			}
			if (requestUri.equals(getAppStr(appStrType::getDiagnosticsDataStr))) {
				return handleGetJsonResponse(diagnosticsData);
			}
			if (requestUri.equals(getAppStr(appStrType::getMqttDataStr))) {
				return handleGetJsonResponse(mqttData);
			}
			if (requestUri.equals(getAppStr(appStrType::getGpioDataStr))) {
				return handleGetJsonResponse(gpioData);
			}
			if (requestUri.equals(getAppStr(appStrType::getPowerMgmtDataStr))) {
				return handleGetJsonResponse(powerMgmtData);
			}
			if (requestUri.equals(getAppStr(appStrType::getPowerMgmtScheduleDataStr))) {
				return handleGetJsonResponse(powerMgmtScheduleData);
			}
			if (requestUri.equals(getAppStr(appStrType::getTotpQrCodeDataStr))) {
				return handleGetJsonResponse(totpQrCodeUriData);
			}
			if (requestUri.equals(getAppStr(appStrType::getPeripheralsDataStr))) {
				return handleGetJsonResponse(peripheralsData);
			}

			return false;
		}

	};

	//create the server on the configured port
	webserver = new ESP8266WebServer(appConfigData.webserverPort);

	//add the multi-handler classes
	webserver->addHandler(new StaticResourceHandler());
	webserver->addHandler(new HtmlGetResourceHandler());
	webserver->addHandler(new AjaxGetResourceHandler());

	webserver->on(getAppStr(appStrType::firmwareUpload).c_str(), HTTP_POST, []() {
		if (Update.hasError() || firmwareUploadFail) {
			webserver->send(200, getAppStr(appStrType::textContentType), getAppStr(appStrType::fail));
		} else {
			persistConfig();
		}
	}, handleFirmwareUpload);

	webserver->on(getAppStr(appStrType::logonStr).c_str(), HTTP_POST, handleLogon);

	webserver->on(getAppStr(appStrType::logoff).c_str(), handleLogoff);

	webserver->on(getAppStr(appStrType::network).c_str(), HTTP_POST, handleSaveNetSettings);

	webserver->on(getAppStr(appStrType::dns).c_str(), HTTP_POST, handleSaveDnsSettings);

	/*
	 * fileSystemEdit SPIFFS functionality START
	 */
	//list directory
	webserver->on(getAppStr(appStrType::listStr).c_str(), HTTP_GET, handleFileList);

	//load editor
	webserver->on(getAppStr(appStrType::edit).c_str(), HTTP_GET, []() {
		returnResource(getAppStr(appStrType::fileSystemEditHtm));
	});

	//create a file
	webserver->on(getAppStr(appStrType::edit).c_str(), HTTP_PUT, handleFileCreate);
	//delete a file
	webserver->on(getAppStr(appStrType::edit).c_str(), HTTP_DELETE, handleFileDelete);
	//upload a file
	webserver->on(getAppStr(appStrType::edit).c_str(), HTTP_POST,
		[](){
		webserver->send(200, getAppStr(appStrType::textContentType), EMPTY_STR);
		},
		handleFileUpload);
	/*
	 * fileSystemEdit SPIFFS functionality END
	 */

	webserver->on(getAppStr(appStrType::reboot).c_str(), HTTP_GET, handleReboot);

	webserver->on(getAppStr(appStrType::reset).c_str(), HTTP_GET, handleReset);

	webserver->on(getAppStr(appStrType::webserverStr).c_str(), HTTP_POST, handleSaveWebserverSettings);

	webserver->on(getAppStr(appStrType::regional).c_str(), HTTP_POST, handleSaveRegionalSettings);

	webserver->on(getAppStr(appStrType::mqttStr).c_str(), HTTP_POST, handleSaveMqttSettings);

	webserver->on(getAppStr(appStrType::gpioStr).c_str(), HTTP_POST, handleSaveGpioSettings);

	webserver->on(getAppStr(appStrType::digitalWriteStr).c_str(), HTTP_GET, handleDigitalWrite);

	webserver->on(getAppStr(appStrType::analogWriteStr).c_str(), HTTP_GET, handleAnalogWrite);

	webserver->on(getAppStr(appStrType::peripheralWriteStr).c_str(), HTTP_GET, handlePeripheralWrite);

	webserver->on(getAppStr(appStrType::powerMgmtStr).c_str(), HTTP_POST, handleSavePowerMgmtSettings);

	webserver->on(getAppStr(appStrType::addPowerMgmtScheduleStr).c_str(), HTTP_POST, handleAddPowerMgmtSchedule);

	webserver->on(getAppStr(appStrType::removePowerMgmtScheduleStr).c_str(), HTTP_POST, handleRemovePowerMgmtSchedule);

	webserver->on(getAppStr(appStrType::password).c_str(), HTTP_POST, handleChangePassword);

	webserver->on(getAppStr(appStrType::submitTotpTokenStr).c_str(), HTTP_POST, handleSubmitTotpToken);

	webserver->on(getAppStr(appStrType::addPeripheralStr).c_str(), HTTP_POST, handleAddPeripheral);

	webserver->on(getAppStr(appStrType::removePeripheralStr).c_str(), HTTP_POST, handleRemovePeripheral);


	/*webserver->on("/test", HTTP_GET, []() {

	});*/


	//404 handler
	webserver->onNotFound([]() {
		webserver->send(404, getAppStr(appStrType::textContentType), webserver->uri() + getAppStr(appStrType::fileNotFound));
	});


	//track and gather useful request headers
	const char * headerkeys[] =
	{
		"Accept-Encoding",
		"If-Modified-Since",
		"Cookie",
		"X-SSL",
		"X-SSL-WebserverPort",
		"X-SSL-WebsocketPort",
		"X-Forwarded-For"
	};
	size_t headerKeyCount = sizeof(headerkeys) / sizeof(char*);
	webserver->collectHeaders(headerkeys, headerKeyCount);


	//start the webserver and SPIFFS to service client requests
	webserver->begin();
	bool serverStarted = SPIFFS.begin();

	lastHeapCheck = millis();
	rebootTimeStamp = 0;

	APP_SERIAL_DEBUG("Webserver started\n");

	return serverStarted;
}

void ICACHE_FLASH_ATTR processWebClientRequests() {

	webserver->handleClient();

	if (millis() - lastHeapCheck > 1000) {

		APP_SERIAL_DEBUG("Free heap: %d\n", ESP.getFreeHeap());

		lastHeapCheck = millis();
	}

	if (rebootTimeStamp != 0 && millis() > rebootTimeStamp) {
		ESP.restart();
	}
}
