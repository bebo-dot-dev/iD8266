/*
 * MQTTManager.h
 *
 *  Created on: 20 Apr 2016
 *      Author: joe
 */

#ifndef MQTTMANAGER_H_
#define MQTTMANAGER_H_

#include <ESP8266WiFi.h>
#include "/home/joe/git/pubsubclient/src/PubSubClient.h"
#include "flashAppData.h"

#define MQTT_RECONNECT_INTERVAL 4000

class MQTTManager {
public:
	ICACHE_FLASH_ATTR MQTTManager(WiFiClient* wifiClient, uint8_t serverIP[4], uint16_t serverPort, const char* username, const char* password);
	ICACHE_FLASH_ATTR virtual ~MQTTManager();
	bool ICACHE_FLASH_ATTR processMqtt();
	bool ICACHE_FLASH_ATTR publish(const char* topic, uint8_t value);
	bool connected;
	bool ICACHE_FLASH_ATTR connect();
	uint8_t connectAttemptCount = 1;
	unsigned long lastReconnectAttempt = 0;
	bool ICACHE_FLASH_ATTR getClientConnectedState();
private:
	PubSubClient* mqttClient;
	IPAddress mqttServerIP;
	const char* mqttUsername;
	const char* mqttPassword;
	static ICACHE_FLASH_ATTR void mqttCallback(char* topic, byte* payload, unsigned int length);
};

#endif /* MQTTMANAGER_H_ */
