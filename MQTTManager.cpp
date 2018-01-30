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

ICACHE_FLASH_ATTR MQTTManager::MQTTManager(
    WiFiClient* wifiClient,
    uint8_t serverIP[4],
    uint16_t serverPort,
    const char* username,
    const char* password,
    const char* deviceHostName) {

    mqttServerIP = IPAddress(serverIP[0], serverIP[1], serverIP[2], serverIP[3]);
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

    // attempt to connect
    if (mqttClient->connect(mqttDeviceHostName, mqttUsername, mqttPassword)) {

        APP_SERIAL_DEBUG("MQTT client connected\n");
        connectAttemptCount = 0;
        lastReconnectAttempt = 0;

        String strTopic;
        for (uint8_t deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {
            subscribe(appConfigData.gpio.digitals[deviceIdx].pinMode, deviceIdx);
        }
        connected = true;

    }

    return connected;
}

bool ICACHE_FLASH_ATTR MQTTManager::subscribe(digitalMode pinMode, uint8_t deviceIdx) {
    //subscribe an output to an MQTT IN topic
    if (mqttClient->connected()) {
        String strTopic;
        switch (pinMode) {
            case digitalMode::digitalOutput:
            case digitalMode::digitalAnalogOutputPwm:
                strTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::D) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::IN);
                mqttClient->unsubscribe(strTopic.c_str());
                return mqttClient->subscribe(strTopic.c_str());
            case digitalMode::digitalOutputPeripheral:
                peripheralData *peripheral;
                peripheral = GPIOMngr.getPeripheralByIdx(deviceIdx);
                if (peripheral != NULL) {
                    switch(peripheral->type){
                        case peripheralType::digistatMk2:
                            strTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::P) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::IN);
                            mqttClient->unsubscribe(strTopic.c_str());
                            return mqttClient->subscribe(strTopic.c_str());
                        case peripheralType::homeEasySwitch:
                            strTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::P) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::VID) + String(peripheral->virtualDeviceId) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::IN);
                            return mqttClient->subscribe(strTopic.c_str());
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }
    }

    return false;
}

bool ICACHE_FLASH_ATTR MQTTManager::publish(ioType type, uint8_t deviceIdx, peripheralType pType) {
    if (mqttClient->connected()) {

        String mqttTopic;
        String mqttPayload;

        if (pType == peripheralType::unspecified) {

            switch (type) {
            case ioType::digital:
                mqttTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::D) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::OUT);
                mqttPayload = String(appConfigData.gpio.digitals[deviceIdx].lastValue);
                break;
            default:
                mqttTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::A) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::OUT);
                mqttPayload = String(appConfigData.gpio.analogRawValue);
                break;
            }

            return publish(mqttTopic, mqttPayload);

        } else {

            peripheralData *peripheral = &appConfigData.device.peripherals[deviceIdx];

            switch (peripheral->type) {

                case peripheralType::digistatMk2:
                    mqttTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::P) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::OUT);
                    mqttPayload = String(peripheral->base.lastValue);
                    return publish(mqttTopic, mqttPayload);

                case peripheralType::dht22:
                    //both DHT22 temperature and humidity are mqtt published serially
                    mqttTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::P) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::TEMPERATURE) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::OUT);
                    mqttPayload = String(peripheral->lastAnalogValue1, 1);
                    bool published;
                    published = publish(mqttTopic, mqttPayload);

                    if (published) {
                        delay(20);
                        mqttTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::P) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::HUMIDITY) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::OUT);
                        mqttPayload = String(peripheral->lastAnalogValue2, 1);
                        published &= publish(mqttTopic, mqttPayload);
                    }
                    return published;

                case peripheralType::homeEasySwitch:
                    mqttTopic = String(mqttDeviceHostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::P) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::VID) + String(peripheral->virtualDeviceId) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::OUT);
                    mqttPayload = String(peripheral->base.lastValue);
                    return publish(mqttTopic, mqttPayload);

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

//handles an MQTT callback as write to an output
void ICACHE_FLASH_ATTR MQTTManager::mqttCallback(char* topic, byte* payload, unsigned int length) {

    String inboundTopic = String(topic);
    String outputTopic;

    char* outputValueStr((char*) payload);
    uint8_t outputValue = strtoul(outputValueStr, 0, 10);

    for (uint8_t deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {

        switch (appConfigData.gpio.digitals[deviceIdx].pinMode) {
            case digitalMode::digitalOutput:
            case digitalMode::digitalAnalogOutputPwm:

                outputTopic = String(appConfigData.hostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::D) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::IN);

                if (inboundTopic.equals(outputTopic)) {

                    if (appConfigData.gpio.digitals[deviceIdx].pinMode == digitalOutput) {
                        GPIOMngr.gpioDigitalWrite(deviceIdx, outputValue);
                    } else {
                        GPIOMngr.gpioAnalogWrite(deviceIdx, outputValue);
                    }
                    return;
                }
                break;
            default:
                break;
            }
    }

    for (uint8_t deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {

        peripheralData *peripheral = GPIOMngr.getPeripheralByIdx(deviceIdx);
        if (peripheral != NULL) {
            switch(peripheral->type){
                case peripheralType::digistatMk2:
                    outputTopic = String(appConfigData.hostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::P) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::IN);
                    if (inboundTopic.equals(outputTopic)) {
                        GPIOMngr.peripheralWrite(deviceIdx, peripheral->type, 0, outputValue);
                        return;
                    }
                    break;
                case peripheralType::homeEasySwitch:
                    outputTopic = String(appConfigData.hostName) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::P) + String(deviceIdx) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::VID) + String(peripheral->virtualDeviceId) + getAppStr(appStrType::FORWARD_SLASH) + getAppStr(appStrType::IN);
                    if (inboundTopic.equals(outputTopic)) {
                        GPIOMngr.peripheralWrite(deviceIdx, peripheral->type, peripheral->virtualDeviceId, outputValue);
                        return;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}
