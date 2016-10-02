/*
 * MQTTManager.cpp
 *
 *  Created on: 20 Apr 2016
 *      Author: joe
 */

#include "MQTTManager.h"
#include "utils.h"
#include "flashAppData.h"
#include "GPIOManager.h"
#include "NetworkServices.h"


ICACHE_FLASH_ATTR MQTTManager::MQTTManager(WiFiClient* wifiClient, uint8_t serverIP[4], uint16_t serverPort, const char* username, const char* password) {

	mqttServerIP = IPAddress(serverIP[0],serverIP[1],serverIP[2],serverIP[3]);
	mqttUsername = username;
	mqttPassword = password;
	connectAttemptCount = 1;
	lastReconnectAttempt = 0;
	connected = false;

	WiFiClient **client = &wifiClient;
	mqttClient = new PubSubClient(**client);

	mqttClient->setServer(mqttServerIP, serverPort);
	mqttClient->setCallback(mqttCallback);

	processMqtt(); //try an initial connect
}

ICACHE_FLASH_ATTR MQTTManager::~MQTTManager() {
	mqttClient->disconnect();
}

bool ICACHE_FLASH_ATTR MQTTManager::processMqtt() {

	bool processed = false;

	if (!mqttClient->connected()) {

		unsigned long now = millis();

		uint16_t reconnectIncrement = (connectAttemptCount * 1000); //up to a max of 60 seconds
		uint16_t reconnectInterval = (MQTT_RECONNECT_INTERVAL + reconnectIncrement);
		if ((now - lastReconnectAttempt) >= reconnectInterval || lastReconnectAttempt == 0) {

			lastReconnectAttempt = now;

			connected = connect();

			if (connectAttemptCount <= 60) {
				connectAttemptCount++;
			}
		}

	} else {
		processed = mqttClient->loop();
	}

	return processed;
}

bool ICACHE_FLASH_ATTR MQTTManager::getClientConnectedState() {
	return mqttClient->connected();
}

bool ICACHE_FLASH_ATTR MQTTManager::connect() {

	bool connected = false;

	APP_SERIAL_DEBUG("MQTT client connect attempt\n");

	// Attempt to connect
	if (mqttClient->connect(appConfigData.hostName, mqttUsername, mqttPassword)) {

		APP_SERIAL_DEBUG("MQTT client connected\n");
		lastReconnectAttempt = 0;

		//subscribe outputs to in topics
		for(int i = 0; i < MAX_GPIO; i++) {
			switch(appConfigData.gpio.digitalIO[i].digitalPinMode)
			{
				case digitalOutput:
				case digitalAnalogOutputPwm:
					mqttClient->subscribe(appConfigData.gpio.digitalIO[i].digitalName);
					break;
				default:
					break;
			}
		}
		connected = true;

	}

	return connected;
}

bool ICACHE_FLASH_ATTR MQTTManager::publish(const char* topic, uint8_t value) {

	String payload = String(value);
	return mqttClient->publish(topic, payload.c_str());
}

//handles an mqtt callback as a potential write to an output, if enabled on the pin
void ICACHE_FLASH_ATTR MQTTManager::mqttCallback(char* topic, byte* payload, unsigned int length) {

	char *ptr_char = NULL;

	for(int i = 0; i < MAX_GPIO; i++) {
		switch(appConfigData.gpio.digitalIO[i].digitalPinMode)
		{
			case digitalOutput:
			case digitalAnalogOutputPwm:

				ptr_char = appConfigData.gpio.digitalIO[i].digitalName;

				if (strcmp(topic, ptr_char) == 0) {
					char* outputValueStr((char*)payload);

					uint8_t outputValue = strtoul(outputValueStr, 0, 10);

					if (appConfigData.gpio.digitalIO[i].digitalPinMode == digitalOutput) {
						GPIOMngr.gpioDigitalWrite(i, outputValue, true);
					} else {
						GPIOMngr.gpioAnalogWrite(i, outputValue, true);
					}
					break;
				}
				break;
			default:
				break;
		}
	}
}

