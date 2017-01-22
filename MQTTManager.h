/*
 * MQTTManager.h
 *
 *  Created on: 20 Apr 2016
 *      Author: joe
 */

#ifndef MQTTMANAGER_H_
#define MQTTMANAGER_H_

#include <ESP8266WiFi.h>
#include "PubSubClient.h"
#include "GPIOData.h"

#define MQTT_RECONNECT_INTERVAL 4000

class MQTTManager {
public:
	bool connected;
	uint8_t connectAttemptCount = 0;
	unsigned long lastReconnectAttempt = 0;

	static constexpr const char* const TOPIC_SLASH = "/";
	static constexpr const char* const GPIO_TOPIC = "D";
	static constexpr const char* const ANALOG_TOPIC = "A";
	static constexpr const char* const PERIPHERAL_TOPIC = "P";
	static constexpr const char* const IN_TOPIC = "In";
	static constexpr const char* const OUT_TOPIC = "Out";
	static constexpr const char* const TEMPERATURE_TOPIC = "Temperature";
	static constexpr const char* const HUMIDITY_TOPIC = "Humidity";

	ICACHE_FLASH_ATTR MQTTManager(WiFiClient* wifiClient, uint8_t serverIP[4], uint16_t serverPort, const char* username, const char* password, const char* deviceHostName);
	ICACHE_FLASH_ATTR virtual ~MQTTManager();
	bool ICACHE_FLASH_ATTR processMqtt();
	bool ICACHE_FLASH_ATTR subscribe(
		digitalMode pinMode,
		uint8_t deviceIdx);
	bool ICACHE_FLASH_ATTR publish(
		ioType type,
		uint8_t deviceIdx,
		peripheralType pType);
	bool ICACHE_FLASH_ATTR publish(const String &mqttTopic, const String &mqttPayload);
	bool ICACHE_FLASH_ATTR connect();
	bool ICACHE_FLASH_ATTR getClientConnectedState();
private:
	PubSubClient* mqttClient;
	IPAddress mqttServerIP;
	const char* mqttUsername;
	const char* mqttPassword;
	const char* mqttDeviceHostName;
	static ICACHE_FLASH_ATTR void mqttCallback(char* topic, byte* payload, unsigned int length);
};

#endif /* MQTTMANAGER_H_ */
