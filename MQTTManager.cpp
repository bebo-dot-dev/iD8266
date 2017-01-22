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
	connectAttemptCount = 0;
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
		connectAttemptCount = 0;
		lastReconnectAttempt = 0;

		String strTopic;
		for(uint8_t deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {
			subscribe(appConfigData.gpio.digitals[deviceIdx].pinMode, deviceIdx);
		}
		connected = true;

	}

	return connected;
}

bool ICACHE_FLASH_ATTR MQTTManager::subscribe (
	digitalMode pinMode,
	uint8_t deviceIdx)
{
	//subscribe an output to an mqtt IN topic

	if (mqttClient->connected()) {

		String strTopic;
		switch(pinMode)
		{
			case digitalMode::digitalOutput:
			case digitalMode::digitalAnalogOutputPwm:
				strTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + MQTTManager::GPIO_TOPIC + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::IN_TOPIC;
				mqttClient->unsubscribe(strTopic.c_str());
				return mqttClient->subscribe(strTopic.c_str());
			case digitalMode::digitalOutputPeripheral:
				strTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + MQTTManager::PERIPHERAL_TOPIC + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::IN_TOPIC;
				mqttClient->unsubscribe(strTopic.c_str());
				return mqttClient->subscribe(strTopic.c_str());
			default:
				break;
		}

	}

	return false;
}

bool ICACHE_FLASH_ATTR MQTTManager::publish(
	ioType type,
	uint8_t deviceIdx,
	peripheralType pType)
{
	if (mqttClient->connected()) {

		String mqttTopic;
		String mqttPayload;

		if(pType == peripheralType::unspecified) {

			switch(type)
			{
				case ioType::digital:
					mqttTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + String(MQTTManager::GPIO_TOPIC) + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::OUT_TOPIC;
					mqttPayload = String(appConfigData.gpio.digitals[deviceIdx].lastValue);
					break;
				default:
					mqttTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + String(MQTTManager::ANALOG_TOPIC) + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::OUT_TOPIC;
					mqttPayload = String(appConfigData.gpio.analogRawValue);
					break;
			}

			return publish(mqttTopic, mqttPayload);

		} else {

			peripheralData *peripheral = &appConfigData.device.peripherals[deviceIdx];

			switch(peripheral->type) {

				case peripheralType::digistatMk2:
					mqttTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + String(MQTTManager::PERIPHERAL_TOPIC) + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::OUT_TOPIC;
					mqttPayload = String(peripheral->base.lastValue);
					return publish(mqttTopic, mqttPayload);

				case peripheralType::dht22:
					//both DHT22 temperature and humidity are mqtt published serially
					mqttTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + String(MQTTManager::PERIPHERAL_TOPIC) + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::TEMPERATURE_TOPIC + MQTTManager::TOPIC_SLASH + MQTTManager::OUT_TOPIC;
					mqttPayload = String(peripheral->lastAnalogValue1, 1);
					bool published;
					published = publish(mqttTopic, mqttPayload);

					mqttTopic = String(mqttDeviceHostName) + MQTTManager::TOPIC_SLASH + String(MQTTManager::PERIPHERAL_TOPIC) + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::HUMIDITY_TOPIC + MQTTManager::TOPIC_SLASH + MQTTManager::OUT_TOPIC;
					mqttPayload = String(peripheral->lastAnalogValue2, 1);
					published &= publish(mqttTopic, mqttPayload);
					return published;

				default:
					break;
			}
		}
	}

	return false;
}

bool ICACHE_FLASH_ATTR MQTTManager::publish(const String &mqttTopic, const String &mqttPayload) {

	if (mqttClient->connected()) {

		APP_SERIAL_DEBUG("MQTTManager::publish %s, %s\n", mqttTopic.c_str(), mqttPayload.c_str());
		return mqttClient->publish(mqttTopic.c_str(), mqttPayload.c_str());

	}

	return false;
}

//handles an mqtt callback as write to an output
void ICACHE_FLASH_ATTR MQTTManager::mqttCallback(char* topic, byte* payload, unsigned int length) {

	String inboundTopic = String(topic);
	String outputTopic;

	char* outputValueStr((char*)payload);
	uint8_t outputValue = strtoul(outputValueStr, 0, 10);

	for(uint8_t deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {

		switch(appConfigData.gpio.digitals[deviceIdx].pinMode)
		{
			case digitalMode::digitalOutput:
			case digitalMode::digitalAnalogOutputPwm:

				outputTopic = String(appConfigData.hostName) + MQTTManager::TOPIC_SLASH + MQTTManager::GPIO_TOPIC + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::IN_TOPIC;

				if (inboundTopic.equals(outputTopic)) {

					if (appConfigData.gpio.digitals[deviceIdx].pinMode == digitalOutput) {
						GPIOMngr.gpioDigitalWrite(deviceIdx, outputValue, true);
					} else {
						GPIOMngr.gpioAnalogWrite(deviceIdx, outputValue, true);
					}
					break;
				}
				break;
			case digitalMode::digitalOutputPeripheral:

				outputTopic = String(appConfigData.hostName) + MQTTManager::TOPIC_SLASH + MQTTManager::PERIPHERAL_TOPIC + String(deviceIdx) + MQTTManager::TOPIC_SLASH + MQTTManager::IN_TOPIC;

				if (inboundTopic.equals(outputTopic)) {
					GPIOMngr.gpioDigitalWrite(deviceIdx, outputValue, true);
					break;
				}

				break;
			default:
				break;
		}
	}
}

