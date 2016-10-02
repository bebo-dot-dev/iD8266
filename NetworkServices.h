#ifndef NETWORK_SERVICES_H

#define NETWORK_SERVICES_H

#include "Ntp.h"
#include "SocketServer.h"
#include "MQTTManager.h"


struct loggingStatusData {
	int lastLoggedDigitalIOVal[MAX_GPIO];
	int lastLoggedAnalogueVal;
};
struct thingSpeakLoggingStatusData : loggingStatusData {
	unsigned long lastThingSpeakLog;
	int lastThingSpeakResponseCode;
};

struct httpLoggingStatusData : loggingStatusData {
	unsigned long lastHttpLog;
	String lastUri;
	bool lastCallOk;
};

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
	thingSpeakLoggingStatusData thingSpeakLoggingData;
	httpLoggingStatusData httpLoggingData;

	Ntp* ntpManager;
	SocketServer* socketServer;
	MQTTManager* mqttManager;
	PowerManager* powerManager;

	void ICACHE_FLASH_ATTR startNetworkServices(rst_info *resetInfo);
	bool ICACHE_FLASH_ATTR setNtp(uint8_t server, int8_t tz, time_t syncSecs);
	String ICACHE_FLASH_ATTR deviceMacStr();
	String ICACHE_FLASH_ATTR devMacLastSix();
	bool ICACHE_FLASH_ATTR mqttConnected();
	bool ICACHE_FLASH_ATTR broadcastGPIOChange(
		gpioType type,
		uint8_t pinIdx,
		bool mqttOutEnabled = true);
	static String ICACHE_FLASH_ATTR getSanitizedHttpLoggingUri(String uri);
	static String ICACHE_FLASH_ATTR urlEncode(const char* stringToEncode);
	bool ICACHE_FLASH_ATTR performHttpGetRequest(WiFiClient* wifiClient, String host, String request);
	void processRequests();
private:
	bool broadcastingGPIOChange;
	WiFiClient* networkServicesWifiClient;
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
		const char* password);

	bool ICACHE_FLASH_ATTR isSingleCycleEnabled();
	void ICACHE_FLASH_ATTR initLoggingStatusData();

	int ICACHE_FLASH_ATTR logToThingSpeak(
		gpioType type,
		uint8_t pinIdx);

	bool ICACHE_FLASH_ATTR logToHttpServer(
		gpioType type,
		uint8_t pinIdx);
};

extern NetworkServicesManager NetworkServiceManager;

#endif //NETWORK_SERVICES_H
