/*
 networkServices.ino - introduces network related helper functions
 */

#include "flashAppData.h"
#include "webserver.h"
#include "utils.h"
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <stddef.h>
#include "Print.h"

#include "NetworkServices.h"
#include "Ntp.h"
#include "MQTTManager.h"
#include "/home/joe/git/thingspeak-arduino/src/ThingSpeak.h"

NetworkServicesManager NetworkServiceManager;
DNSServer* dnsServer;

ICACHE_FLASH_ATTR NetworkServicesManager::NetworkServicesManager() {

	webserverStarted = false;
	socketserverStarted = false;
	mdnsStarted = false;
	dnsStarted = false;
	ntpEnabled = false;
	mqttEnabled = false;
	networkServicesWifiClient = NULL;
	ntpManager = NULL;
	socketServer = NULL;
	mqttManager = NULL;

	broadcastingGPIOChange = false;

	initLoggingStatusData();
}

ICACHE_FLASH_ATTR NetworkServicesManager::~NetworkServicesManager() {

}

bool ICACHE_FLASH_ATTR NetworkServicesManager::isSingleCycleEnabled() {
	return appConfigData.powerMgmt.enabled &&
			appConfigData.powerMgmt.onLength == powerOnLength::zeroLength &&
			!powerManager->delayedSleep;
}

void ICACHE_FLASH_ATTR NetworkServicesManager::initLoggingStatusData() {

	thingSpeakLoggingData.lastThingSpeakLog = (millis() - 15000);
	httpLoggingData.lastHttpLog = millis();

	for(int pinIdx = 0; pinIdx < MAX_GPIO; pinIdx++) {
		thingSpeakLoggingData.lastLoggedDigitalIOVal[pinIdx] = UNDEFINED_GPIO;
		httpLoggingData.lastLoggedDigitalIOVal[pinIdx] = UNDEFINED_GPIO;
	}

	thingSpeakLoggingData.lastLoggedAnalogueVal = UNDEFINED_GPIO;
	httpLoggingData.lastLoggedAnalogueVal = UNDEFINED_GPIO;

}

void ICACHE_FLASH_ATTR NetworkServicesManager::startNetworkServices(rst_info *resetInfo) {

	if (appConfigData.initialized) {

		powerManager = new PowerManager(resetInfo);

		WiFi.mode(WiFiMode::WIFI_OFF);

		bool wifiModeSet = WiFi.mode(appConfigData.wifiMode);

		if (wifiModeSet) {
			switch (appConfigData.wifiMode) {
				case WIFI_AP:
					APP_SERIAL_DEBUG("wifiMode is WIFI_AP\n");
					connectWifiAccessPoint();
					break;
				case WIFI_AP_STA:
					APP_SERIAL_DEBUG("wifiMode is WIFI_AP_STA\n");
					connectWifiAccessPoint();
					connectWifiStation();
					break;
				case WIFI_STA:
					APP_SERIAL_DEBUG("wifiMode is WIFI_STA\n");
					connectWifiStation();
					break;
				default:
					APP_SERIAL_DEBUG("wifiMode is WIFI_OFF\n");
					break;
			}

			if (appConfigData.ntpEnabled) {
				ntpEnabled = setNtp(
					appConfigData.ntpServer,
					appConfigData.ntpTimeZone,
					appConfigData.ntpSyncInterval);
			}

			powerManager->logBoot();

			bool singleCycleEnabled = isSingleCycleEnabled();

			if (!singleCycleEnabled)
			{
				webserverStarted = configureWebServices();
				if (webserverStarted) {

					if (appConfigData.mdnsEnabled)
						mdnsStarted = startMulticastDNS();
					if (appConfigData.dnsEnabled)
						dnsStarted = startDNS();

					socketServer = new SocketServer(appConfigData.websocketServerPort);
					socketserverStarted = true;
				}
			}

			if (appConfigData.mqttSystemEnabled && !singleCycleEnabled)
				mqttEnabled = startMqtt(
					getWifiClient(),
					appConfigData.mqttServerBrokerIp,
					appConfigData.mqttServerBrokerPort,
					appConfigData.mqttUsername,
					appConfigData.mqttPassword);

			if (appConfigData.thingSpeakEnabled) {
				getWifiClient();
				WiFiClient **client = &networkServicesWifiClient;
				ThingSpeak.begin( **client );
			}
		}

		GPIOMngr.initialize();
	}
}

void ICACHE_FLASH_ATTR NetworkServicesManager::connectWifiStation() {

	APP_SERIAL_DEBUG("STA Network Mode: %s\n", appConfigData.networkApIpMode == Dhcp ? "DHCP" : "Static IP");

	if (appConfigData.networkApIpMode == Static) {

		IPAddress staticIp(
			appConfigData.networkApStaticIp[0],
			appConfigData.networkApStaticIp[1],
			appConfigData.networkApStaticIp[2],
			appConfigData.networkApStaticIp[3]);

		IPAddress subnet(
			appConfigData.networkApSubnet[0],
			appConfigData.networkApSubnet[1],
			appConfigData.networkApSubnet[2],
			appConfigData.networkApSubnet[3]);

		IPAddress gateway(
			appConfigData.networkApGatewayIp[0],
			appConfigData.networkApGatewayIp[1],
			appConfigData.networkApGatewayIp[2],
			appConfigData.networkApGatewayIp[3]);

		IPAddress dns(
			appConfigData.networkApDnsIp[0],
			appConfigData.networkApDnsIp[1],
			appConfigData.networkApDnsIp[2],
			appConfigData.networkApDnsIp[3]);

		IPAddress dns2(
			appConfigData.networkApDns2Ip[0],
			appConfigData.networkApDns2Ip[1],
			appConfigData.networkApDns2Ip[2],
			appConfigData.networkApDns2Ip[3]);

		WiFi.config(staticIp, gateway, subnet, dns, dns2);
	}

	bool has_ssid = strcmp(appConfigData.networkApSSID, EMPTY_STR) != 0;
	bool has_pwd = strcmp(appConfigData.networkApPwd, EMPTY_STR) != 0;
	bool wifiConnect = false;

	if ((has_ssid) || (has_pwd)) {
		if ((has_ssid) && (has_pwd)) {
			wifiConnect = true;
			WiFi.begin(appConfigData.networkApSSID, appConfigData.networkApPwd);
		} else if (has_ssid) {
			wifiConnect = true;
			WiFi.begin(appConfigData.networkApSSID);
		}
	}

	if (wifiConnect) {
		int connectCntr = 0;
		while (WiFi.status() != WL_CONNECTED && connectCntr <= 40) {
			connectCntr++;
			delay(500);
			APP_SERIAL_DEBUG(".");
		}

		if (WiFi.status() != WL_CONNECTED) {
			//connection attempt failed after timeout - reboot and try again
			ESP.restart();
		}

		APP_SERIAL_DEBUG("\nSTA IP Address: %s\n", WiFi.localIP().toString().c_str());

		WiFi.setAutoReconnect(true);

	}

}

void ICACHE_FLASH_ATTR NetworkServicesManager::connectWifiAccessPoint() {

	bool has_ssid = strcmp(appConfigData.deviceApSSID, EMPTY_STR) != 0;
	bool has_pwd = strcmp(appConfigData.deviceApPwd, EMPTY_STR) != 0;

	if ((has_ssid) || (has_pwd)) {

		if ((has_ssid) && (has_pwd)) {

			WiFi.softAP(appConfigData.deviceApSSID, appConfigData.deviceApPwd,appConfigData.deviceApChannel);

		} else if (has_ssid) {

			WiFi.softAP(appConfigData.deviceApSSID, NULL, appConfigData.deviceApChannel);

		}
		APP_SERIAL_DEBUG("AP IP Address: %s\n", WiFi.softAPIP().toString().c_str());
		APP_SERIAL_DEBUG("AP SSID: %s\n", appConfigData.deviceApSSID);
	}
}

/*
 returns a mac address that looks like: 18FE34D4C782
 */
String ICACHE_FLASH_ATTR NetworkServicesManager::deviceMacStr() {

	byte macBytes[6];

	WiFi.macAddress(macBytes);
	String macStr = EMPTY_STR;
	for (int i = 0; i < 6; ++i) {
		macStr += String(macBytes[i], HEX);
	}

	macStr.toUpperCase();
	return macStr;
}

String ICACHE_FLASH_ATTR NetworkServicesManager::devMacLastSix() {
	String macStr = deviceMacStr();
	if (macStr.length() >= 6)
		macStr.remove(0, 6);
	return macStr;
}

bool ICACHE_FLASH_ATTR NetworkServicesManager::startMulticastDNS() {
	bool mdnsStarted = MDNS.begin(appConfigData.hostName);
	MDNS.addService(getAppStr(appStrType::httpProtocol), getAppStr(appStrType::tcpProtocol), appConfigData.webserverPort);
	return mdnsStarted;
}

bool ICACHE_FLASH_ATTR NetworkServicesManager::startDNS() {

	dnsServer = new DNSServer();
	dnsServer->setTTL(appConfigData.dnsTTL);
	String dnsDomain;

	if (appConfigData.dnsCatchAll) {
		dnsDomain = getAppStr(appStrType::asterix);
		dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
	} else {
		dnsDomain = appConfigData.hostName;
		dnsServer->setErrorReplyCode(DNSReplyCode::ServerFailure); //reduce client dns requests not targeting this domain
	}

	bool dnsStarted = false;

	switch (appConfigData.wifiMode) {
	case WIFI_AP:
		dnsStarted = dnsServer->start(appConfigData.dnsPort, dnsDomain, WiFi.softAPIP());
		break;
	case WIFI_AP_STA:
		if (WiFi.status() != WL_CONNECTED) {
			dnsStarted = dnsServer->start(appConfigData.dnsPort, dnsDomain, WiFi.softAPIP());
		} else {
			dnsStarted = dnsServer->start(appConfigData.dnsPort, dnsDomain, WiFi.localIP());
		}
		break;
	case WIFI_STA:
		if (WiFi.status() == WL_CONNECTED) {
			dnsStarted = dnsServer->start(appConfigData.dnsPort, dnsDomain, WiFi.localIP());
		}
		break;
	default:
		break;
	}

	return dnsStarted;
}

bool ICACHE_FLASH_ATTR NetworkServicesManager::setNtp(uint8_t server, int8_t tz, time_t syncSecs)
{
	//ntp is only enabled for STA connected devices
	bool enabled = false;
	switch (appConfigData.wifiMode) {
	case WIFI_AP_STA:
	case WIFI_STA:
		if (WiFi.status() == WL_CONNECTED) {
			//ntp setup
			if (ntpEnabled) {
				delete ntpManager;
			}
			ntpManager = new Ntp(server, tz, syncSecs);
			enabled = true;
		}
		break;
	default:
		enabled = false;
		break;
	}
	return enabled;
}

WiFiClient* ICACHE_FLASH_ATTR NetworkServicesManager::getWifiClient()
{
	if (!networkServicesWifiClient) {
		networkServicesWifiClient = new WiFiClient();
	}
	return networkServicesWifiClient;
}

bool ICACHE_FLASH_ATTR NetworkServicesManager::startMqtt(
	WiFiClient* wifiClient,
	uint8_t serverIP[4],
	uint16_t serverPort,
	const char* username,
	const char* password)
{

	bool enabled = false;

	if (WiFi.status() == WL_CONNECTED) {

		if (serverIP[0] > 0 || serverIP[1] > 0 || serverIP[2] > 0 || serverIP[3] > 0) {
			mqttManager = new MQTTManager(wifiClient, serverIP, serverPort, username, password);
			enabled = true;
		}

	}

	return enabled;
}

bool ICACHE_FLASH_ATTR NetworkServicesManager::mqttConnected() {

	bool connected = false;

	if (mqttManager != NULL) {

		connected = mqttManager->getClientConnectedState();

	}

	return connected;
}

/*
 * broadcasts details of a GPIO change to interested parties including:
 * 1) websocket clients
 * 2) mqtt clients subscribed to this device's mqtt topics
 * 3) thingspeak
 * 4) some arbitrary http server
 */
bool ICACHE_FLASH_ATTR NetworkServicesManager::broadcastGPIOChange(
	gpioType type,
	uint8_t pinIdx,
	bool mqttOutEnabled)
{
	APP_SERIAL_DEBUG("NetworkServicesManager::broadcastGPIOChange\n");

	broadcastingGPIOChange = true;

	bool broadcast = false;
	bool mqttPublish = false;
	int tsResultCode = 0;
	bool httpLogged = false;

	char* pinName;
	int value;
	bool thingSpeakEnabled;
	bool httpLoggingEnabled;

	switch(type)
	{
		case digital:
			pinName = appConfigData.gpio.digitalIO[pinIdx].digitalName;
			value = appConfigData.gpio.digitalIO[pinIdx].lastValue;
			thingSpeakEnabled = appConfigData.gpio.digitalIO[pinIdx].logToThingSpeak;
			httpLoggingEnabled = appConfigData.gpio.digitalIO[pinIdx].httpLoggingEnabled;
			break;
		default:
			pinName = appConfigData.gpio.analogName;
			value = appConfigData.gpio.analogRawValue;
			thingSpeakEnabled = appConfigData.gpio.analogLogToThingSpeak;
			httpLoggingEnabled = appConfigData.gpio.analogHttpLoggingEnabled;
			break;
	}


	if (socketserverStarted) {
		broadcast = socketServer->broadcastGPIOChange(type, pinIdx);
	}

	if ((mqttEnabled) && (mqttManager->connected) && (mqttOutEnabled)) {
		mqttPublish = mqttManager->publish(pinName, value);
	}

	if (thingSpeakEnabled && appConfigData.thingSpeakEnabled) {
		tsResultCode = logToThingSpeak(type, pinIdx);
	}

	if (httpLoggingEnabled && appConfigData.httpLoggingEnabled) {
		httpLogged = logToHttpServer(type, pinIdx);
	}

	broadcastingGPIOChange = false;

	return broadcast || mqttPublish || tsResultCode == 200 || httpLogged;

}

int ICACHE_FLASH_ATTR NetworkServicesManager::logToThingSpeak(
	gpioType type,
	uint8_t pinIdx)
{
	APP_SERIAL_DEBUG("NetworkServicesManager::logToThingSpeak\n");

	int response = 0;

	unsigned long now = millis();

	if ((now - thingSpeakLoggingData.lastThingSpeakLog) >= 15000) { // ThingSpeak call velocity is >= every 15 seconds

		bool valueChanged =
			type == digital
			? appConfigData.gpio.digitalIO[pinIdx].lastValue != thingSpeakLoggingData.lastLoggedDigitalIOVal[pinIdx]
			: appConfigData.gpio.analogRawValue != thingSpeakLoggingData.lastLoggedAnalogueVal;

		if ((valueChanged) && (WiFi.status() == WL_CONNECTED)) {

			ThingSpeak.setField(1, pinIdx);
			ThingSpeak.setField(2, type);

			if (ntpEnabled) {
				ThingSpeak.setField(5, ntpManager->iso8601DateTime());
			}

			switch(type)
			{
				case digital:

					thingSpeakLoggingData.lastLoggedDigitalIOVal[pinIdx] = appConfigData.gpio.digitalIO[pinIdx].lastValue;

					ThingSpeak.setField(3, appConfigData.gpio.digitalIO[pinIdx].digitalName);
					ThingSpeak.setField(4, appConfigData.gpio.digitalIO[pinIdx].lastValue);

					response = ThingSpeak.writeFields(
							appConfigData.gpio.digitalIO[pinIdx].thingSpeakChannel,
							appConfigData.gpio.digitalIO[pinIdx].thingSpeakApiKey);

					break;
				default:

					thingSpeakLoggingData.lastLoggedAnalogueVal = appConfigData.gpio.analogRawValue;

					ThingSpeak.setField(3, appConfigData.gpio.analogName);
					ThingSpeak.setField(4, appConfigData.gpio.analogRawValue);

					ThingSpeak.setField(6, (float)appConfigData.gpio.analogVoltage);
					ThingSpeak.setField(7, (float)appConfigData.gpio.analogCalcVal);
					ThingSpeak.setField(8, GPIOManager::getUnitOfMeasureStr(appConfigData.gpio.analogUnit));

					response = ThingSpeak.writeFields(
							appConfigData.gpio.analogThingSpeakChannel,
							appConfigData.gpio.analogThingSpeakApiKey);

					break;
			}

			thingSpeakLoggingData.lastThingSpeakResponseCode = response;
			thingSpeakLoggingData.lastThingSpeakLog = now;
		}
	}
	else {
		response = -1;
	}

	return response;
}

bool ICACHE_FLASH_ATTR NetworkServicesManager::logToHttpServer(
	gpioType type,
	uint8_t pinIdx)
{
	long now = millis();

	bool isSingleCycle = isSingleCycleEnabled();

	if (((now - httpLoggingData.lastHttpLog) >= 60000) || //logging timeout expired
			(type == digital) || //digital GPIOs are always logged
			(isSingleCycle)) { //all power management single cycle events are always logged

		bool valueChanged = isSingleCycle ? true :
			type == digital
			? appConfigData.gpio.digitalIO[pinIdx].lastValue != httpLoggingData.lastLoggedDigitalIOVal[pinIdx]
			: appConfigData.gpio.analogRawValue != httpLoggingData.lastLoggedAnalogueVal;

		if(valueChanged)
			httpLoggingData.lastCallOk = false;

		getWifiClient();

		if ((valueChanged) && (WiFi.status() == WL_CONNECTED) && (networkServicesWifiClient->connect(appConfigData.httpLoggingHost, 80))) {

			String url = getSanitizedHttpLoggingUri(appConfigData.httpLoggingUri);

			String space = getAppStr(appStrType::spaceStr);
			String qsData;

			//hashtags (identifiers)
			qsData += "#" + String(appConfigData.hostName) + space;
			qsData += type == digital ? "#D" : "#A";
			qsData += String(pinIdx) + space;

			//pin name and value(s)
			switch(type)
			{
				case digital:
					qsData +=
							String(appConfigData.gpio.digitalIO[pinIdx].digitalName) +
							getAppStr(appStrType::equals) +
							String(appConfigData.gpio.digitalIO[pinIdx].lastValue);
					qsData +=
							appConfigData.gpio.digitalIO[pinIdx].lastValue == 0
							? getAppStr(appStrType::digitalOff)
							: getAppStr(appStrType::digitalOn);
					break;
				default:
					qsData +=
							String(appConfigData.gpio.analogName) +
							getAppStr(appStrType::equals) +
							String(appConfigData.gpio.analogCalcVal) +
							space +
							GPIOManager::getUnitOfMeasureStr(appConfigData.gpio.analogUnit);
					break;
			}

			if (appConfigData.ntpEnabled && ntpManager->syncResponseReceived) {
				qsData += getAppStr(appStrType::deviceTime) + ntpManager->iso8601DateTime();
			}
			qsData += getAppStr(appStrType::deviceUpTime) + Ntp::getDeviceUptimeString();

			String uri = url + urlEncode(qsData.c_str());
			String host = String(appConfigData.httpLoggingHost);

			bool httpOk = performHttpGetRequest(networkServicesWifiClient, host, uri);
			if (!httpOk) {
				httpLoggingData.lastCallOk = false;
				httpLoggingData.lastHttpLog = now;
			} else {
				httpLoggingData.lastCallOk = true;
				httpLoggingData.lastHttpLog = now;
			}
		}
	}

	return httpLoggingData.lastCallOk;
}

bool ICACHE_FLASH_ATTR NetworkServicesManager::performHttpGetRequest(WiFiClient* wifiClient, String host, String request) {

	httpLoggingData.lastUri = request;

	wifiClient->println(getAppStr(appStrType::httpGET) + httpLoggingData.lastUri + getAppStr(appStrType::httpVersion));
	wifiClient->println(getAppStr(appStrType::httpHost) + host);
	wifiClient->println(getAppStr(appStrType::httpAccept));
	wifiClient->println(getAppStr(appStrType::httpConnectionClose));
	wifiClient->println();

	int timeout = millis() + 5000;

	while (wifiClient->available() == 0) {
		//loop waiting for a reply
		if (timeout - millis() <= 0) {
			APP_SERIAL_DEBUG("HTTP call timeout\n");
			wifiClient->stop();
			return false;
		}
		yield();
	}

	// Read all the lines of the response from server and dump them to serial
	while (wifiClient->available()) {
		String response = wifiClient->readStringUntil('\r');
		APP_SERIAL_DEBUG(response.c_str());
	}

	return true;

}

String ICACHE_FLASH_ATTR NetworkServicesManager::getSanitizedHttpLoggingUri(String uri) {

	String url = String(uri);
	if (!(url.startsWith(getAppStr(appStrType::forwardSlash)))) {
		url = getAppStr(appStrType::forwardSlash) + url;
	}

	if (url.indexOf(getAppStr(appStrType::questionMark)) == -1) {
		url += getAppStr(appStrType::questionMark);
	} else {
		if (!url.endsWith(getAppStr(appStrType::equalsSign))) {
			url += getAppStr(appStrType::equalsSign);
		}
	}
	return url;
}

String ICACHE_FLASH_ATTR NetworkServicesManager::urlEncode(const char* stringToEncode) {

	const char *hex = "0123456789abcdef";
	String encodedStr = "";

	while (*stringToEncode != '\0') {
		if( ('a' <= *stringToEncode && *stringToEncode <= 'z')
				|| ('A' <= *stringToEncode && *stringToEncode <= 'Z')
				|| ('0' <= *stringToEncode && *stringToEncode <= '9') ) {
			encodedStr += *stringToEncode;
		} else {
			encodedStr += '%';
			encodedStr += hex[*stringToEncode >> 4];
			encodedStr += hex[*stringToEncode & 15];
		}
		stringToEncode++;
	}
	return encodedStr;

}

void ICACHE_FLASH_ATTR NetworkServicesManager::processRequests() {

	if (webserverStarted)
		processWebClientRequest();

	if (mdnsStarted)
		MDNS.update();

	if (dnsStarted)
		dnsServer->processNextRequest();

	if (socketserverStarted)
		socketServer->handleRequests();

	if (ntpEnabled)
		ntpManager->processTime();

	if (mqttEnabled)
		mqttManager->processMqtt();

	if (!broadcastingGPIOChange)
		GPIOMngr.readInputs();

	powerManager->processPowerEvents();

}

