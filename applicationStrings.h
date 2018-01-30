/*
 * ApplicationStrings.h
 *
 *  Created on: 13 May 2016
 *      Author: Joe
 */

#ifndef APPLICATIONSTRINGS_H_

#define APPLICATIONSTRINGS_H_

#include <Arduino.h>
#include <pgmspace.h>

/*
 * STRING_TABLE defines all strings and their associated enum value in the preprocessor generated
 * appStrType enum and appStrings[] array
 */
#define STRING_TABLE \
	ENTRY(jjs8266InitMarkerStr, "JJS8266_INIT_V0") \
	ENTRY(lastModifiedTimestampStr, "Fri, 04 Mar 2016 21:15:00 GMT") \
	ENTRY(defaultDeviceNamePrefixStr, "iD8266_") \
	ENTRY(defaultPwdStr, "password1234") \
	ENTRY(defaultLocaleStr, "en-GB") \
	ENTRY(defaultAnalogNameStr, "analogInput") \
	ENTRY(celciusStr, "°C") \
	ENTRY(fahrenheitStr, "°F") \
	ENTRY(mm, "mm") \
	ENTRY(cm, "cm") \
	ENTRY(m, "m") \
	ENTRY(km, "km") \
	ENTRY(in, "in") \
	ENTRY(ft, "ft") \
	ENTRY(mi, "mi") \
	ENTRY(ml, "ml") \
	ENTRY(cl, "cl") \
	ENTRY(l, "l") \
	ENTRY(gal, "gal") \
	ENTRY(mg, "mg") \
	ENTRY(g, "g") \
	ENTRY(kg, "kg") \
	ENTRY(oz, "oz") \
	ENTRY(lb, "lb") \
	ENTRY(t, "t") \
	ENTRY(BTU, "BTU") \
	ENTRY(mV, "mV") \
	ENTRY(v, "V") \
	ENTRY(mA, "mA") \
	ENTRY(A, "A") \
	ENTRY(ohms, "Ω") \
	ENTRY(W, "W") \
	ENTRY(D, "D") \
	ENTRY(P, "P") \
	ENTRY(DeviceClass_WiFiController, "iD8266 WiFi Controller") \
	ENTRY(dot, ".") \
	ENTRY(httpProtocol, "http") \
	ENTRY(tcpProtocol, "tcp") \
	ENTRY(asterix, "*") \
	ENTRY(equals, " == ") \
	ENTRY(digitalOn, " (ON)") \
	ENTRY(digitalOff, " (OFF)") \
	ENTRY(FORWARD_SLASH, "/") \
	ENTRY(questionMark, "?") \
	ENTRY(equalsSign, "=") \
	ENTRY(hyphen, "-") \
	ENTRY(colon, ":") \
	ENTRY(pipe, "|") \
	ENTRY(comma, ",") \
	ENTRY(commaAndSpace, ", ") \
	ENTRY(zero, "0") \
	ENTRY(newline, "\n") \
	ENTRY(deviceTime, "\r\nDevice time: ") \
	ENTRY(deviceUpTime, "\r\nDevice up time: ") \
	ENTRY(httpGET, "GET ") \
	ENTRY(httpVersion, " HTTP/1.1") \
	ENTRY(httpHost, "Host: ") \
	ENTRY(httpAccept, "Accept: */*") \
	ENTRY(httpConnectionClose, "Connection: close") \
	ENTRY(ntp1, "pool.ntp.org") \
	ENTRY(ntp2, "europe.pool.ntp.org") \
	ENTRY(ntp3, "time-a.timefreq.bldrdoc.gov") \
	ENTRY(ntp4, "time-b.timefreq.bldrdoc.gov") \
	ENTRY(ntp5, "time-c.timefreq.bldrdoc.gov") \
	ENTRY(ntp6, "chronos.csr.net") \
	ENTRY(ntpDays, " days, ") \
	ENTRY(ntpHours, " hours, ") \
	ENTRY(ntpMins, " mins, ") \
	ENTRY(ntpSecs, " secs") \
	ENTRY(openBrace, "{") \
	ENTRY(closeBrace, "}") \
	ENTRY(escapedComma, "\",") \
	ENTRY(escapedQuote, "\"") \
	ENTRY(gpioTypeStr, "\"GPIOType\": \"") \
	ENTRY(escapedIdxStr, "\"Idx\": \"") \
	ENTRY(escapedValueStr, "\"Value\": \"") \
	ENTRY(escapedValue2Str, "\"Value2\": \"") \
	ENTRY(voltageStr, "\"Voltage\": \"") \
	ENTRY(calculatedStr, "\"Calculated\": \"") \
	ENTRY(sslReverseProxy, "X-SSL") \
	ENTRY(sslReverseProxyWebserverPort, "X-SSL-WebserverPort") \
	ENTRY(sslReverseProxyWebsocketPort, "X-SSL-WebsocketPort") \
	ENTRY(sslReverseProxyForwardedFor, "X-Forwarded-For") \
	ENTRY(sslProxyEnabledTrue, ",\"sslProxyEnabled\": \"true\"") \
	ENTRY(sslProxyEnabledFalse, ",\"sslProxyEnabled\": \"false\"") \
	ENTRY(sslProxyWebserverPort, ",\"sslProxyWebserverPort\": \"") \
	ENTRY(sslProxyWebsocketPort, ",\"sslProxyWebsocketPort\": \"") \
	ENTRY(httpScheme, "http://") \
	ENTRY(httpsScheme, "https://") \
	ENTRY(sessionId, "sessionId") \
	ENTRY(sessionLocator, "sessionId=") \
	ENTRY(sessionPlaceholder, "sessionId=%s|") \
	ENTRY(cookie, "Cookie") \
	ENTRY(serverStr, "Server") \
	ENTRY(cacheControl, "cache-control") \
	ENTRY(cacheControlPolicyZeroMaxAge, "public, max-age=0") \
	ENTRY(cacheControlPolicy300MaxAge, "public, max-age=300") \
	ENTRY(lastModified, "Last-Modified") \
	ENTRY(noCacheControlPolicy, "public, max-age=0, no-cache, no-store") \
	ENTRY(htmExtension, ".htm") \
	ENTRY(htmlExtension, ".html") \
	ENTRY(cssExtension, ".css") \
	ENTRY(jsExtension, ".js") \
	ENTRY(pngExtension, ".png") \
	ENTRY(gifExtension, ".gif") \
	ENTRY(jpgExtension, ".jpg") \
	ENTRY(icoExtension, ".ico") \
	ENTRY(xmlExtension, ".xml") \
	ENTRY(pdfExtension, ".pdf") \
	ENTRY(zipExtension, ".zip") \
	ENTRY(gzExtension, ".gz") \
	ENTRY(binExtension, ".bin") \
	ENTRY(idHtmlExtension, ".cm") \
	ENTRY(htmlContentType, "text/html") \
	ENTRY(cssContentType, "text/css") \
	ENTRY(jsContentType, "application/javascript") \
	ENTRY(pngContentType, "image/png") \
	ENTRY(gifContentType, "image/gif") \
	ENTRY(jpegContentType, "image/jpeg") \
	ENTRY(icoContentType, "image/x-icon") \
	ENTRY(xmlContentType, "text/xml") \
	ENTRY(pdfContentType, "application/x-pdf") \
	ENTRY(zipContentType, "application/x-zip") \
	ENTRY(gzipContentType, "application/x-gzip") \
	ENTRY(textContentType, "text/plain") \
	ENTRY(jsonContentType, "application/json") \
	ENTRY(textJsonContentType, "text/json") \
	ENTRY(acceptEncoding, "Accept-Encoding") \
	ENTRY(ifModifiedSince, "If-Modified-Since") \
	ENTRY(gzipStr, "gzip") \
	ENTRY(spiffsHit, "SPIFFS hit for ") \
	ENTRY(loginMin, "login.min.htm") \
	ENTRY(locationStr, "Location") \
	ENTRY(logonStr, "/logon") \
	ENTRY(loginStr, "/login.cm") \
	ENTRY(slashLoginMin, "/login.min.htm") \
	ENTRY(updateDone, "/updateDone.min.htm") \
	ENTRY(setCookie, "Set-Cookie") \
	ENTRY(resetCookie, "sessionId=0;") \
	ENTRY(intPlaceholderStr, "%d") \
	ENTRY(escapedWifiMode, "\"wifiMode\": \"") \
	ENTRY(escapedApSSID, "\"apSSID\": \"") \
	ENTRY(escapedApPwd, "\"apPwd\": \"") \
	ENTRY(escapedApChannel, "\"apChannel\": \"") \
	ENTRY(escapedNetApSSID, "\"netApSSID\": \"") \
	ENTRY(escapedNetApPwd, "\"netApPwd\": \"") \
	ENTRY(escapedNetApIpMode, "\"netApIpMode\": \"") \
	ENTRY(ipAddressTemplateStr, "%u.%u.%u.%u") \
	ENTRY(escapedNetApStaticIp, "\"netApStaticIp\": \"") \
	ENTRY(escapedNetApSubnet, "\"netApSubnet\": \"") \
	ENTRY(escapedNetApGatewayIp, "\"netApGatewayIp\": \"") \
	ENTRY(escapedNetApDnsIp, "\"netApDnsIp\": \"") \
	ENTRY(escapedNetApDns2Ip, "\"netApDns2Ip\": \"") \
	ENTRY(escapedHostName, "\"hostName\": \"") \
	ENTRY(mdnsEnabledTrue, "\"mdnsEnabled\": \"true\",") \
	ENTRY(mdnsEnabledFalse, "\"mdnsEnabled\": \"false\",") \
	ENTRY(dnsEnabledTrue, "\"dnsEnabled\": \"true\",") \
	ENTRY(dnsEnabledFalse, "\"dnsEnabled\": \"false\",") \
	ENTRY(escapedDnsTTL, "\"dnsTTL\": \"") \
	ENTRY(escapedDnsPort, "\"dnsPort\": \"") \
	ENTRY(dnsCatchAllTrue, "\"dnsCatchAll\": \"true\"") \
	ENTRY(dnsCatchAllFalse, "\"dnsCatchAll\": \"false\"") \
	ENTRY(escapedAdminPwd, "\"adminPwd\": \"") \
	ENTRY(escapedWebserverPort, "\"webserverPort\": \"") \
	ENTRY(escapedWebsocketServerPort, "\"websocketServerPort\": \"") \
	ENTRY(includeServerHeaderTrue, "\"includeServerHeader\": \"true\",") \
	ENTRY(includeServerHeaderFalse, "\"includeServerHeader\": \"false\",") \
	ENTRY(escapedServerHeader, "\"serverHeader\": \"") \
	ENTRY(ntpEnabledTrue, "\"ntpEnabled\": \"true\",") \
	ENTRY(ntpEnabledFalse, "\"ntpEnabled\": \"false\",") \
	ENTRY(escapedNtpServer, "\"ntpServer\": \"") \
	ENTRY(escapedNtpTimeZone, "\"ntpTimeZone\": \"") \
	ENTRY(escapedNtpSyncInterval, "\"ntpSyncInterval\": \"") \
	ENTRY(escapedNtpLocale, "\"ntpLocale\": \"") \
	ENTRY(syncResponseReceivedTrue, "\"syncResponseReceived\": \"true\",") \
	ENTRY(syncResponseReceivedFalse, "\"syncResponseReceived\": \"false\",") \
	ENTRY(escapedYear, "\"year\": \"") \
	ENTRY(escapedMonth, "\"month\": \"") \
	ENTRY(escapedDay, "\"day\": \"") \
	ENTRY(escapedHour, "\"hour\": \"") \
	ENTRY(escapedMinute, "\"minute\": \"") \
	ENTRY(escapedSecond, "\"second\": \"") \
	ENTRY(escapedLastSyncYear, "\"lastSyncYear\": \"") \
	ENTRY(escapedLastSyncMonth, "\"lastSyncMonth\": \"") \
	ENTRY(escapedLastSyncDay, "\"lastSyncDay\": \"") \
	ENTRY(escapedLastSyncHour, "\"lastSyncHour\": \"") \
	ENTRY(escapedLastSyncMinute, "\"lastSyncMinute\": \"") \
	ENTRY(escapedLastSyncSecond, "\"lastSyncSecond\": \"") \
	ENTRY(escapedLastSyncRetries, "\"lastSyncRetries\": \"") \
	ENTRY(escapedWeekday, "\"weekday\": \"") \
	ENTRY(escapedFreeHeap, "\"freeHeap\": \"") \
	ENTRY(escapedFirmwareVersion, "\"firmwareVersion\": \"") \
	ENTRY(escapedSdkVersion, "\"sdkVersion\": \"") \
	ENTRY(escapedCoreVersion, "\"coreVersion\": \"") \
	ENTRY(escapedDeviceClass, "\"deviceClass\": \"") \
	ENTRY(escapedDeviceClassString, "\"deviceClassString\": \"") \
	ENTRY(escapedDeviceMac, "\"deviceMac\": \"") \
	ENTRY(escapedDeviceIp, "\"deviceIp\": \"") \
	ENTRY(escapedDeviceApIp, "\"deviceApIp\": \"") \
	ENTRY(escapedClientIp, "\"clientIp\": \"") \
	ENTRY(escapedBinaryBytes, "\"binaryBytes\": \"") \
	ENTRY(escapedFreeBytes, "\"freeBytes\": \"") \
	ENTRY(escapedDeviceUpTime, "\"deviceUpTime\": \"") \
	ENTRY(wdtResetCount, "\"wdtResetCount\": \"") \
	ENTRY(exceptionResetCount, "\"exceptionResetCount\": \"") \
	ENTRY(softWdtResetCount, "\"softWdtResetCount\": \"") \
	ENTRY(mqttSystemEnabledTrue, "\"mqttSystemEnabled\": \"true\",") \
	ENTRY(mqttSystemEnabledFalse, "\"mqttSystemEnabled\": \"false\",") \
	ENTRY(escapedMqttServerBrokerIp, "\"mqttServerBrokerIp\": \"") \
	ENTRY(escapedMqttServerBrokerPort, "\"mqttServerBrokerPort\": \"") \
	ENTRY(escapedMqttUsername, "\"mqttUsername\": \"") \
	ENTRY(escapedMqttPassword, "\"mqttPassword\": \"") \
	ENTRY(escapedWebserver, "\"webserver\": ") \
	ENTRY(mqttConnectedTrue, "\"mqttConnected\": \"true\"") \
	ENTRY(mqttConnectedFalse, "\"mqttConnected\": \"false\"") \
	ENTRY(escapedName, "\"Name\": \"") \
	ENTRY(escapedMode, "\"Mode\": \"") \
	ENTRY(escapedDefault, "\"Default\": \"") \
	ENTRY(A0Name, "\"A0Name\": \"") \
	ENTRY(A0Mode, "\"A0Mode\": \"") \
	ENTRY(A0Value, "\"A0Value\": \"") \
	ENTRY(A0Voltage, "\"A0Voltage\": \"") \
	ENTRY(A0Offset, "\"A0Offset\": \"") \
	ENTRY(A0Multiplier, "\"A0Multiplier\": \"") \
	ENTRY(A0Unit, "\"A0Unit\": \"") \
	ENTRY(A0UnitStr, "\"A0UnitStr\": \"") \
	ENTRY(A0Calculated, "\"A0Calculated\": \"") \
	ENTRY(escapedLowerName, "\"name\": \"") \
	ENTRY(analogName, "\"analogName\": \"") \
	ENTRY(runtimeDataStr, "\"runtimeData\":") \
	ENTRY(netSettingsStr, "\"netSettings\":") \
	ENTRY(dnsSettingsStr, "\"dnsSettings\":") \
	ENTRY(webserverSettingsStr, "\"webserverSettings\":") \
	ENTRY(regionalSettingsStr, "\"regionalSettings\":") \
	ENTRY(mqttDataStr, "\"mqttData\":") \
	ENTRY(gpioDataStr, "\"gpioData\":") \
	ENTRY(accessDenied, "Access denied") \
	ENTRY(gpiodtlMinHtm, "/gpiodtl.min.htm") \
	ENTRY(wifiMode, "wifiMode") \
	ENTRY(apSSID, "apSSID") \
	ENTRY(apPwd, "apPwd") \
	ENTRY(apChannel, "apChannel") \
	ENTRY(netApSSID, "netApSSID") \
	ENTRY(netApPwd, "netApPwd") \
	ENTRY(netApIpMode, "netApIpMode") \
	ENTRY(netApStaticIp, "netApStaticIp") \
	ENTRY(netApSubnet, "netApSubnet") \
	ENTRY(netApGatewayIp, "netApGatewayIp") \
	ENTRY(netApDnsIp, "netApDnsIp") \
	ENTRY(netApDns2Ip, "netApDns2Ip") \
	ENTRY(onStr, "on") \
	ENTRY(hostName, "hostName") \
	ENTRY(mdnsEnabled, "mdnsEnabled") \
	ENTRY(dnsEnabled, "dnsEnabled") \
	ENTRY(dnsTTL, "dnsTTL") \
	ENTRY(dnsPort, "dnsPort") \
	ENTRY(dnsCatchAll, "dnsCatchAll") \
	ENTRY(updateDoneMinHtm, "/updateDone.min.htm") \
	ENTRY(adminPwd, "adminPwd") \
	ENTRY(webserverPort, "webserverPort") \
	ENTRY(websocketServerPort, "websocketServerPort") \
	ENTRY(includeServerHeader, "includeServerHeader") \
	ENTRY(serverHeader, "serverHeader") \
	ENTRY(ntpEnabled, "ntpEnabled") \
	ENTRY(ntpServer, "ntpServer") \
	ENTRY(ntpTimeZone, "ntpTimeZone") \
	ENTRY(ntpSyncInterval, "ntpSyncInterval") \
	ENTRY(ntpLocale, "ntpLocale") \
	ENTRY(PinMode, "PinMode") \
	ENTRY(Name, "Name") \
	ENTRY(Default, "Default") \
	ENTRY(mqttSystemEnabled, "mqttSystemEnabled") \
	ENTRY(mqttServerBrokerIp, "mqttServerBrokerIp") \
	ENTRY(mqttServerBrokerPort, "mqttServerBrokerPort") \
	ENTRY(mqttUsername, "mqttUsername") \
	ENTRY(mqttPassword, "mqttPassword") \
	ENTRY(a0PinMode, "a0PinMode") \
	ENTRY(a0Name, "a0Name") \
	ENTRY(a0Offset, "a0Offset") \
	ENTRY(a0Multiplier, "a0Multiplier") \
	ENTRY(a0Unit, "a0Unit") \
	ENTRY(valueStr, "value") \
	ENTRY(jsonResultOk, "{\"result\":\"OK\"}") \
	ENTRY(jsonResultFailed, "{\"result\":\"FAILED\"}") \
	ENTRY(badArgs, "BAD ARGS") \
	ENTRY(dirStr, "dir") \
	ENTRY(fileStr, "file") \
	ENTRY(badPath, "BAD PATH") \
	ENTRY(fileExists, "FILE EXISTS") \
	ENTRY(createFailed, "CREATE FAILED") \
	ENTRY(edit, "/edit") \
	ENTRY(fail, "FAIL") \
	ENTRY(firmwareUpload, "/firmwareUpload") \
	ENTRY(indexStr, "/index.min.htm") \
	ENTRY(logoff, "/logoff") \
	ENTRY(network, "/network") \
	ENTRY(getNetSettingsStr, "/getNetSettings") \
	ENTRY(dns, "/dns") \
	ENTRY(getDnsSettingsStr, "/getDnsSettings") \
	ENTRY(listStr, "/list") \
	ENTRY(fileSystemEditHtm, "/fileSystemEdit.htm") \
	ENTRY(getHomePageDataStr, "/getHomePageData") \
	ENTRY(reboot, "/reboot") \
	ENTRY(reset, "/reset") \
	ENTRY(webserverStr, "/webserver") \
	ENTRY(getWebserverSettingsStr, "/getWebserverSettings") \
	ENTRY(regional, "/regional") \
	ENTRY(getRegionalSettingsStr, "/getRegionalSettings") \
	ENTRY(diagnostics, "/diagnostics") \
	ENTRY(getDiagnosticsDataStr, "/getDiagnosticsData") \
	ENTRY(mqttStr, "/mqtt") \
	ENTRY(getMqttDataStr, "/getMqttData") \
	ENTRY(gpioStr, "/gpio") \
	ENTRY(getGpioDataStr, "/getGpioData") \
	ENTRY(gpioArray, "\"gpio\":[") \
	ENTRY(gpioArrayWithCommaPrefix, ",\"gpio\":[") \
	ENTRY(gpiodtl, "/gpiodtl") \
	ENTRY(digitalWriteStr, "/digitalWrite") \
	ENTRY(analogWriteStr, "/analogWrite") \
	ENTRY(peripheralWriteStr, "/peripheralWrite") \
	ENTRY(getPowerMgmtDataStr, "/getPowerMgmtData") \
	ENTRY(getPowerMgmtScheduleDataStr, "/getPowerMgmtScheduleData") \
	ENTRY(powerMgmtStr, "/powerMgmt") \
	ENTRY(addPowerMgmtScheduleStr, "/addPowerMgmtSchedule") \
	ENTRY(removePowerMgmtScheduleStr, "/removePowerMgmtSchedule") \
	ENTRY(password, "/password") \
	ENTRY(passwordStr, "/password.cm") \
	ENTRY(passwordMin, "password.min.htm") \
	ENTRY(pwd, "pwd") \
	ENTRY(confirmPwd, "confirmPwd") \
	ENTRY(scheduleArray, "\"schedule\":[") \
	ENTRY(escapedEnabledTrue, "\"enabled\": \"true\",") \
	ENTRY(escapedEnabledFalse, "\"enabled\": \"false\",") \
	ENTRY(escapedOnLength, "\"onLength\": \"") \
	ENTRY(escapedOffLength, "\"offLength\": \"") \
	ENTRY(enabled, "enabled") \
	ENTRY(onLength, "onLength") \
	ENTRY(offLength, "offLength") \
	ENTRY(hourStr, "hour") \
	ENTRY(weekdayStr, "weekday") \
	ENTRY(idxStr, "idx") \
	ENTRY(fileNotFound, " FileNotFound") \
	ENTRY(dStr, "d") \
	ENTRY(spaceStr, " ") \
	ENTRY(resourcePostFix, ".min.htm") \
	ENTRY(getJsonPrefix, "/get") \
	ENTRY(automatedWake, "Automated Wake") \
	ENTRY(manualWake, "Manual Wake") \
	ENTRY(scheduledWake, "Scheduled Wake") \
	ENTRY(automatedSleep, "Automated Deep Sleep") \
	ENTRY(scheduledSleep, "Scheduled Deep Sleep") \
	ENTRY(totp, "totp") \
	ENTRY(totpOff, "totpOff") \
	ENTRY(getSecurityDataStr, "/getSecurityData") \
	ENTRY(totpEnabledTrue, "\"totpEnabled\": \"true\"") \
	ENTRY(totpEnabledFalse, "\"totpEnabled\": \"false\"") \
	ENTRY(getTotpQrCodeDataStr, "/getTotpQrCodeData") \
	ENTRY(totpQrCodeUri, "\"totpQrCodeUri\": \"") \
	ENTRY(submitTotpTokenStr, "/submitTotpToken") \
	ENTRY(getPeripheralsDataStr, "/getPeripheralsData") \
	ENTRY(peripheralArray, "\"peripheral\":[") \
	ENTRY(escapedPeripheralType, "\"peripheralType\": \"") \
	ENTRY(escapedPeripheralPin, "\"peripheralPin\": \"") \
	ENTRY(peripheralTypeStr, "peripheralType") \
	ENTRY(addPeripheralStr, "/addPeripheral") \
	ENTRY(removePeripheralStr, "/removePeripheral") \
	ENTRY(virtualDeviceIdStr, "virtualDeviceId") \
	ENTRY(escapedVirtualDeviceId, "\"virtualDeviceId\": \"") \
	ENTRY(pStr, "p") \
	ENTRY(vidStr, "vid") \
	ENTRY(VID, "VID") \
	ENTRY(IN, "IN") \
	ENTRY(OUT, "OUT") \
	ENTRY(TEMPERATURE, "TEMPERATURE") \
	ENTRY(HUMIDITY, "HUMIDITY") \

enum appStrType {
#define ENTRY(enumVal, str) enumVal,
	STRING_TABLE
#undef ENTRY
};

#define ENTRY(enumVal, str) extern const char CLI_STR_ ## enumVal[];
STRING_TABLE
#undef ENTRY

extern const char* const appStrings[];

String getAppStr(appStrType strType);

#endif /* APPLICATIONSTRINGS_H_ */
