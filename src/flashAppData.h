#ifndef FLASH_APP_DATA_H

#define FLASH_APP_DATA_H

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "applicationStrings.h"
#include "GPIOManager.h"
#include "SessionManager.h"
#include "PowerManager.h"
#include "ESP8266TOTP.h"

enum ipMode { Dhcp = 0, Static = 1 };

#define STRMAX 32
#define DOUBLESTRMAX 64
#define LARGESTRMAX 128
#define STRLOCALE 6
#define JJS8266_INIT_MARKER_LENGTH 16
#define EMPTY_STR ""
#define DEFAULT_PWD "password1234"

struct markerData {
	char marker[JJS8266_INIT_MARKER_LENGTH];
};

struct appData {

	bool initialized;

	//describes the wifiMode the device operates as: WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3
	WiFiMode wifiMode;

	//parameters for the device when operating in AP mode
	char deviceApSSID[STRMAX];
	char deviceApPwd[STRMAX];
	byte deviceApChannel;

	//parameters for the device when operating in STA mode
	char networkApSSID[STRMAX];
	char networkApPwd[STRMAX];
	ipMode networkApIpMode; //DHCP or static IP

	//params for static IP mode
	uint8_t networkApStaticIp[4];
	uint8_t networkApSubnet[4];
	uint8_t networkApGatewayIp[4];
	uint8_t networkApDnsIp[4];
	uint8_t networkApDns2Ip[4];

	//web server parameters
	uint16_t webserverPort;
	char adminPwd[STRMAX]; //the admin password
	bool includeServerHeader;
	char serverHeader[STRMAX];
	uint16_t websocketServerPort;

	//ntp parameters
	bool ntpEnabled;
	uint8_t ntpServer;
	int8_t ntpTimeZone;
	time_t ntpSyncInterval;
	char ntpLocale[STRLOCALE];

	//device DNS parameters
	char hostName[STRMAX];
	bool mdnsEnabled;
	bool dnsEnabled;
	uint32_t dnsTTL;
	uint16_t dnsPort;
	bool dnsCatchAll;

	//web sessions survive a reboot
	webSession webSessions[MAX_WEB_SESSIONS];

	//gpio configuration storage data
	gpioStorage gpio;
	peripheralStorage device;

	//mqtt global params
	bool mqttSystemEnabled;
	uint8_t mqttServerBrokerIp[4];
	uint16_t mqttServerBrokerPort;
	char mqttUsername[STRMAX];
	char mqttPassword[STRMAX];

	//power management / deep sleep configuration data
	powerMgmtData powerMgmt;

	//google authenticator secondary auth data
	totpData otp;


};

const size_t markerDataSize = sizeof(markerData);
const size_t appDataSize = sizeof(appData);
const size_t appFlashSize = (markerDataSize + appDataSize);

/*
 * The FlashAppDataManager class that wraps EEPROM and SPIFFS management functionality
 */
class FlashAppDataManager {
private:
	bool eepromInit;
	bool startMarkerPresent();
	void writeStartMarker();
	void initAppDataToFactoryDefaults();
	void clearFlash();
	void formatSPIFFS();
public:
	FlashAppDataManager();
	appData initFlash(bool forceInit = false, bool formatFileSystem = false);
	appData getAppData();
	void setAppData();
};

extern struct markerData appMarkerData;

extern struct appData appConfigData;

extern FlashAppDataManager FlashAppDataMngr;

#endif // FLASH_APP_DATA_H
