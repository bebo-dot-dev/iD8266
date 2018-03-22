#ifndef NETWORK_SERVICES_H

#define NETWORK_SERVICES_H

#include "Ntp.h"
#include "SocketServer.h"
#include "MQTTManager.h"

class NetworkServicesManager {
public:
	ICACHE_FLASH_ATTR NetworkServicesManager();
	ICACHE_FLASH_ATTR virtual ~NetworkServicesManager();

	bool webserverStarted;
	bool socketserverStarted;
	bool mdnsStarted;
	bool dnsStarted;
	bool ntpEnabled;
	bool mqttEnabled;

	Ntp* ntpManager;
	SocketServer* socketServer;
	MQTTManager* mqttManager;
	PowerManager* powerManager;

	bool ICACHE_FLASH_ATTR startNetworkServices(rst_info *resetInfo);
	String ICACHE_FLASH_ATTR deviceMacStr();
	String ICACHE_FLASH_ATTR devMacLastSix();
	bool ICACHE_FLASH_ATTR mqttConnected();
	bool ICACHE_FLASH_ATTR broadcastDeviceStateChange(
		ioType ioType,
		uint8_t deviceIdx,
		peripheralType peripheralType = peripheralType::unspecified);
	void processRequests();
private:
	bool broadcastingGPIOChange;
	WiFiClient* networkServicesWifiClient;
	bool ICACHE_FLASH_ATTR setNtp(uint8_t server, int8_t tz, time_t syncSecs);
	WiFiClient* ICACHE_FLASH_ATTR getWifiClient();
	void ICACHE_FLASH_ATTR connectWifiStation();
	void ICACHE_FLASH_ATTR connectWifiAccessPoint();
	bool ICACHE_FLASH_ATTR startMulticastDNS();
	bool ICACHE_FLASH_ATTR startDNS();
	bool ICACHE_FLASH_ATTR startMqtt(
		WiFiClient* wifiClient,
		uint8_t serverIP[4],
		uint16_t serverPort,
		const char* username,
		const char* password,
		const char* deviceHostName);
};

extern NetworkServicesManager NetworkSvcMngr;

#endif //NETWORK_SERVICES_H
