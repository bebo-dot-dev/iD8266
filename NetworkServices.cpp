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
#include "GPIOData.h"
#include "MQTTManager.h"

NetworkServicesManager NetworkSvcMngr;
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
	powerManager = NULL;

	broadcastingGPIOChange = false;
}

ICACHE_FLASH_ATTR NetworkServicesManager::~NetworkServicesManager() {

}

/*
 * initializes all configured network services and I/O as per
 * currently specified flash persisted configuration
 */
bool ICACHE_FLASH_ATTR NetworkServicesManager::startNetworkServices(rst_info *resetInfo) {

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

			bool singleCycleEnabled = powerManager->isSingleCycleEnabled();

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
					appConfigData.mqttPassword,
					appConfigData.hostName);

			GPIOMngr.initialize();

			return true;
		}
	}
	return false;
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
	for (byte i = 0; i < 6; ++i) {
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
	const char* password,
	const char* deviceHostName)
{

	bool enabled = false;

	if (WiFi.status() == WL_CONNECTED) {

		if (serverIP[0] > 0 || serverIP[1] > 0 || serverIP[2] > 0 || serverIP[3] > 0) {
			mqttManager = new MQTTManager(wifiClient, serverIP, serverPort, username, password, deviceHostName);
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
 * broadcasts details of a device state change to interested parties including:
 * 1) websocket clients
 * 2) mqtt clients subscribed to this device's mqtt topics
 * 3) thingspeak
 * 4) some arbitrary http server
 */
bool ICACHE_FLASH_ATTR NetworkServicesManager::broadcastDeviceStateChange(
	ioType type,
	uint8_t deviceIdx,
	peripheralType pType,
	bool mqttOutEnabled)
{
	APP_SERIAL_DEBUG("NetworkServicesManager::broadcastDeviceStateChange\n");

	broadcastingGPIOChange = true;

	bool broadcast = false;
	bool mqttPublish = false;

	if ((mqttEnabled) && (mqttOutEnabled)) {
		mqttPublish = mqttManager->publish(type, deviceIdx, pType);
	}

	if (socketserverStarted) {
		broadcast = socketServer->broadcastGPIOChange(type, pType, deviceIdx);
	}

	broadcastingGPIOChange = false;

	return broadcast || mqttPublish;

}

void NetworkServicesManager::processRequests() {

	if (webserverStarted)
		processWebClientRequests();

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
		GPIOMngr.processGPIO();

	powerManager->processPowerEvents();

}

