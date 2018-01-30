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
 * broadcasts the value of the given deviceIdx to websocket connected clients
 */
bool ICACHE_FLASH_ATTR SocketServer::broadcastGPIOChange(
	ioType type,
	peripheralType pType,
	uint8_t deviceIdx)
{
	String escapedCommaStr = getAppStr(appStrType::escapedComma);
	String escapedQuoteStr = getAppStr(appStrType::escapedQuote);

	String json = getAppStr(appStrType::openBrace);

	if (type == analog) {
		json += getAppStr(appStrType::voltageStr) + String(appConfigData.gpio.analogVoltage) + escapedCommaStr;
		json += getAppStr(appStrType::calculatedStr) + String(appConfigData.gpio.analogCalcVal) + escapedCommaStr;
	}

	json += getAppStr(appStrType::gpioTypeStr) + String(type) + escapedCommaStr;
	json += getAppStr(appStrType::escapedPeripheralType) + String(pType) + escapedCommaStr;
	json += getAppStr(appStrType::escapedIdxStr) + String(deviceIdx) + escapedCommaStr;

	if (pType == peripheralType::unspecified) {

		int value =
			type == digital
			? appConfigData.gpio.digitals[deviceIdx].lastValue
			: appConfigData.gpio.analogRawValue;

		json += getAppStr(appStrType::escapedValueStr) + String(value) + escapedQuoteStr;

	} else {

		peripheralData *peripheral = &appConfigData.device.peripherals[deviceIdx];

		switch (pType) {

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
	}

	json += getAppStr(appStrType::closeBrace);

	return websocketserver->broadcastTXT(json);
}

void ICACHE_FLASH_ATTR SocketServer::handleRequests() {

	websocketserver->loop();

}

