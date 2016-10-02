/*
 * SocketServer.cpp
 *
 *  Created on: 27 Mar 2016
 *      Author: joe
 */

#include "SocketServer.h"
#include "webserver.h"
#include "GPIOManager.h"
#include "utils.h"

WebSocketsServer* websocketserver;

ICACHE_FLASH_ATTR SocketServer::SocketServer(uint16_t socketServerPort) {
	websocketserver = new WebSocketsServer(socketServerPort);
	const char * headerkeys[] = { "Cookie" };
	size_t headerKeyCount = sizeof(headerkeys) / sizeof(char*);
	websocketserver->onValidateHttpHeader(validateHttpHeader, headerkeys, headerKeyCount);
	websocketserver->begin();
}

ICACHE_FLASH_ATTR SocketServer::~SocketServer() {
	websocketserver->disconnect();
}

bool ICACHE_FLASH_ATTR SocketServer::validateHttpHeader(String headerName, String headerValue) {

	bool valid = true;

	if(headerName.equalsIgnoreCase(getAppStr(appStrType::cookie))) {
		valid = isAuthenticated(headerValue);
	}

	return valid;
}

/*
 * broadcasts the value of the given pinIdx to websocket connected clients
 */
bool ICACHE_FLASH_ATTR SocketServer::broadcastGPIOChange(gpioType type, uint8_t pinIdx) {

	int value =
		type == digital
		? appConfigData.gpio.digitalIO[pinIdx].lastValue
		: appConfigData.gpio.analogRawValue;

	String escapedCommaStr = getAppStr(appStrType::escapedComma);
	String escapedQuoteStr = getAppStr(appStrType::escapedQuote);

	String json = getAppStr(appStrType::openBrace);

	json += getAppStr(appStrType::gpioTypeStr) + String(type) + escapedCommaStr;
	json += getAppStr(appStrType::escapedIdxStr) + String(pinIdx) + escapedCommaStr;
	json += getAppStr(appStrType::escapedValueStr) + String(value) + escapedQuoteStr;

	if (type == analog) {
		json += getAppStr(appStrType::voltageStr) + String(appConfigData.gpio.analogVoltage) + escapedCommaStr;
		json += getAppStr(appStrType::calculatedStr) + String(appConfigData.gpio.analogCalcVal) + escapedQuoteStr;
	}

	json += getAppStr(appStrType::closeBrace);

	return websocketserver->broadcastTXT(json);
}

void ICACHE_FLASH_ATTR SocketServer::handleRequests() {

	websocketserver->loop();

}

