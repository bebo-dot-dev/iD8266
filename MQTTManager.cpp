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


ICACHE_FLASH_ATTR MQTTManager::MQTTManager(WiFiClient* wifiClient, uint8_t serverIP[4], uint16_t serverPort, const char* username, const char* password, const char* deviceHostName) {

	mqttServerIP = IPAddress(serverIP[0],serverIP[1],serverIP[2],serverIP[3]);
	mqttUsername = username;
	mqttPassword = password;
	mqttDeviceHostName = deviceHostName;
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
	if (mqttClient->connect(mqttDeviceHostName, mqttUsername, mqttPassword)) {

		APP_SERIAL_DEBUG("MQTT client connected\n");
		lastReconnectAttempt = 0;

		//subscribe outputs to in topics
		String strTopic;
		for(int i = 0; i < MAX_DEVICES; i++) {
			switch(appConfigData.gpio.digitals[i].pinMode)
			{
				case digitalMode::digitalOutput:
				case digitalMode::digitalAnalogOutputPwm:
					strTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + MQTTManager::GPIO_TOPIC + String(i) + MQTTManager::TOPIC_SLASH + MQTTManager::IN_TOPIC;
					mqttClient->subscribe(strTopic.c_str());
					break;
				case digitalMode::digitalOutputPeripheral:
					strTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + MQTTManager::PERIPHERAL_TOPIC + String(i) + MQTTManager::TOPIC_SLASH + MQTTManager::IN_TOPIC;
					mqttClient->subscribe(strTopic.c_str());
					break;
				default:
					break;
			}
		}
		connected = true;

	}

	return connected;
}

bool ICACHE_FLASH_ATTR MQTTManager::publish(String &topic, String &payload) {

	String strTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + topic + MQTTManager::TOPIC_SLASH + MQTTManager::OUT_TOPIC;
	APP_SERIAL_DEBUG("MQTTManager::publish %s, %s\n", strTopic.c_str(), payload.c_str());
	return mqttClient->publish(strTopic.c_str(), payload.c_str());
}

//handles an mqtt callback as write to an output
void ICACHE_FLASH_ATTR MQTTManager::mqttCallback(char* topic, byte* payload, unsigned int length) {

	String inboundTopic = String(topic);
	String outputTopic;

	char* outputValueStr((char*)payload);
	uint8_t outputValue = strtoul(outputValueStr, 0, 10);

	for(byte i = 0; i < MAX_DEVICES; i++) {

		switch(appConfigData.gpio.digitals[i].pinMode)
		{
			case digitalMode::digitalOutput:
			case digitalMode::digitalAnalogOutputPwm:

				outputTopic = String(appConfigData.hostName) + MQTTManager::TOPIC_SLASH + MQTTManager::GPIO_TOPIC + String(i) + MQTTManager::TOPIC_SLASH + MQTTManager::IN_TOPIC;

				if (inboundTopic.equals(outputTopic)) {

					if (appConfigData.gpio.digitals[i].pinMode == digitalOutput) {
						GPIOMngr.gpioDigitalWrite(i, outputValue, true);
					} else {
						GPIOMngr.gpioAnalogWrite(i, outputValue, true);
					}
					break;
				}
				break;
			case digitalMode::digitalOutputPeripheral:

				outputTopic = String(appConfigData.hostName) + MQTTManager::TOPIC_SLASH + MQTTManager::PERIPHERAL_TOPIC + String(i) + MQTTManager::TOPIC_SLASH + MQTTManager::IN_TOPIC;

				if (inboundTopic.equals(outputTopic)) {
					GPIOMngr.gpioDigitalWrite(i, outputValue, true);
					break;
				}

				break;
			default:
				break;
		}
	}
}

