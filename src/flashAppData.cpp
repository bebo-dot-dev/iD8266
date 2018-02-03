/*
  flashappData.ino - exposes functions for reading/writing appData configuration from flash
  and also functions for managing and maintaining flash memory for the device
*/
#include <FS.h>
#include <Arduino.h>
#include "utils.h"
#include "flashAppData.h"
#include "NetworkServices.h"
#include "PowerManager.h"

const char jjs8266InitMarker[] = "JJS8266_INIT_V3"; //do not change the length of this string (JJS8266_INIT_MARKER_LENGTH)
const char* defaultDeviceNamePrefix = "iD8266_";
const char* defaultLocale = "en-GB";
const char* defaultAnalogName = "analogInput";
const WiFiMode defaultWifiMode = WIFI_AP;

//the global device wide appMarkerData markerData struct
struct markerData appMarkerData;

//the global device wide appConfigData appData struct
struct appData appConfigData;

/*
 * The global FlashAppDataManager instance
 */
FlashAppDataManager FlashAppDataMngr;

/*
 * constructor
 */
FlashAppDataManager::FlashAppDataManager() {
	eepromInit = false;
}

/*
 * populates the global device wide appConfigData appData struct from flash
 */
appData FlashAppDataManager::getAppData() {

	appConfigData = EEPROM.get(markerDataSize, appConfigData);
	return appConfigData;

}

/*
 * writes the global device wide appConfigData appData struct to flash
 */
void FlashAppDataManager::setAppData() {

	appConfigData.initialized = true;
	appConfigData = EEPROM.put(markerDataSize, appConfigData);
	EEPROM.commit();
	APP_SERIAL_DEBUG("EEPROM.commit done\n");

}

/*
 * reads the global device wide appMarkerData markerData struct,
 * compares appMarkerData.marker against jjs8266InitMarker and returns an indicator
 * whether the two values match
 */
bool FlashAppDataManager::startMarkerPresent() {

	appMarkerData = EEPROM.get(0, appMarkerData);
	APP_SERIAL_DEBUG("Flash init marker: %s\n", appMarkerData.marker);
	return (strcmp(appMarkerData.marker, jjs8266InitMarker) == 0);

}

/*
 * writes the global device wide appMarkerData markerData struct to flash
 */
void FlashAppDataManager::writeStartMarker() {

	strncpy(appMarkerData.marker, jjs8266InitMarker, sizeof(jjs8266InitMarker));
	appMarkerData = EEPROM.put(0, appMarkerData);
	APP_SERIAL_DEBUG("Flash init marker: %s\n", appMarkerData.marker);

}

/*
 * ensures flash appMarkerData and appConfigData is correctly initialised
 */
appData FlashAppDataManager::initFlash(bool forceInit, bool formatFileSystem) {

	APP_SERIAL_DEBUG("\ninitFlash start\n");

	if (!eepromInit) {
		EEPROM.begin(SPI_FLASH_SEC_SIZE);
		eepromInit = true;
		APP_SERIAL_DEBUG("EEPROM.begin done\n");
	}


	if ((!startMarkerPresent()) || (forceInit)) {

		APP_SERIAL_DEBUG("flash factory reset\n");

		if(formatFileSystem)
			formatSPIFFS();

		clearFlash();

		//write the start marker
		writeStartMarker();

		//appConfigData reset
		initAppDataToFactoryDefaults();

		//write the appConfigData to flash
		setAppData();

		//reset RTC memory
		PowerManager::initRtcStoreData();

		APP_SERIAL_DEBUG("flash factory reset done\n");

	}
	else
	{
		APP_SERIAL_DEBUG("flash is clean and initialised\n");
		getAppData();
	}

	return appConfigData;
}

/*
 * resets the global device wide appConfigData appData struct to factory default values
 */
void FlashAppDataManager::initAppDataToFactoryDefaults() {

	//**************************************************************************************
	//appConfigData reset to defaults START
	//**************************************************************************************
	appConfigData.wifiMode = defaultWifiMode;

	String deviceName = defaultDeviceNamePrefix;
	deviceName += NetworkSvcMngr.devMacLastSix();

	strncpy(appConfigData.deviceApSSID, deviceName.c_str(), STRMAX);
	strncpy(appConfigData.deviceApPwd, DEFAULT_PWD, STRMAX);
	appConfigData.deviceApChannel = 2;

	strncpy(appConfigData.networkApSSID, EMPTY_STR, STRMAX);
	strncpy(appConfigData.networkApPwd, EMPTY_STR, STRMAX);
	appConfigData.networkApIpMode = Dhcp;

	appConfigData.webserverPort = 80;
	strncpy(appConfigData.adminPwd, DEFAULT_PWD, STRMAX);
	appConfigData.includeServerHeader = false;
	strncpy(appConfigData.serverHeader, deviceName.c_str(), STRMAX);
	appConfigData.websocketServerPort = 81;

	strncpy(appConfigData.hostName, deviceName.c_str(), STRMAX);
	appConfigData.mdnsEnabled = false;
	appConfigData.dnsEnabled = false;
	appConfigData.dnsTTL = 300;
	appConfigData.dnsPort = 53;
	appConfigData.dnsCatchAll = false;

	appConfigData.ntpEnabled = false;
	appConfigData.ntpServer = 0;
	appConfigData.ntpTimeZone = 0;
	appConfigData.ntpSyncInterval = 3600; //default sync is every 1 hour
	strncpy(appConfigData.ntpLocale, defaultLocale, STRLOCALE);

	String name;
	for(byte i = 0; i < MAX_DEVICES; i++) {
		name = DEVICE_IO_PREFIX + String(i);
		strncpy(appConfigData.gpio.digitals[i].name, name.c_str(), STRMAX);
		appConfigData.gpio.digitals[i].pinMode = digitalNotInUse;
		appConfigData.gpio.digitals[i].defaultValue = 0;
		appConfigData.gpio.digitals[i].lastValue = UNDEFINED_GPIO;
	}

	for(byte i = 0; i < MAX_DEVICES; i++) {
		strncpy(appConfigData.device.peripherals[i].base.name , EMPTY_STR, STRMAX);
		appConfigData.device.peripherals[i].base.pinMode = digitalNotInUse;
		appConfigData.device.peripherals[i].base.defaultValue = 0;
		appConfigData.device.peripherals[i].base.lastValue = UNDEFINED_GPIO;

		appConfigData.device.peripherals[i].type = peripheralType::unspecified;
		appConfigData.device.peripherals[i].pinIdx = 0;
		appConfigData.device.peripherals[i].virtualDeviceId = 0;
		appConfigData.device.peripherals[i].lastAnalogValue1 = UNDEFINED_GPIO;
		appConfigData.device.peripherals[i].lastAnalogValue2 = UNDEFINED_GPIO;
	}

	appConfigData.mqttSystemEnabled = false;
	appConfigData.mqttServerBrokerPort = 1883;
	strncpy(appConfigData.mqttUsername, EMPTY_STR, STRMAX);
	strncpy(appConfigData.mqttPassword, EMPTY_STR, STRMAX);

	strncpy(appConfigData.gpio.analogName, defaultAnalogName, STRMAX);
	appConfigData.gpio.analogPinMode = analogNotInUse;
	appConfigData.gpio.analogRawValue = UNDEFINED_GPIO;
	appConfigData.gpio.analogOffset = -0.5;
	appConfigData.gpio.analogMultiplier = 100.0;
	appConfigData.gpio.analogUnit = unitOfMeasure::none;

	appConfigData.powerMgmt.enabled = false;
	appConfigData.powerMgmt.onLength = powerOnLength::infiniteLength;
	appConfigData.powerMgmt.offLength = powerDownLength::oneHour;

	for(byte i = 0; i < MAX_SCHEDULES; i++) {
		appConfigData.powerMgmt.schedules[i].enabled = false;
		appConfigData.powerMgmt.schedules[i].weekday = 2;
		appConfigData.powerMgmt.schedules[i].hour = 0;
		appConfigData.powerMgmt.schedules[i].offLength = powerDownLength::eightHours;
	}

	appConfigData.otp.enabled = false;

	//**************************************************************************************
	//appConfigData reset to defaults END
	//**************************************************************************************

}

/*
 * performs a complete wipe of all 4096 SPI_FLASH_SEC_SIZE bytes of user accessible flash
 */
void FlashAppDataManager::clearFlash() {

	APP_SERIAL_DEBUG("\nclearFlash start\n");
	for (uint16_t i = 0 ; i < SPI_FLASH_SEC_SIZE ; i++) {
		EEPROM.write(i, 0);
		APP_SERIAL_DEBUG(".");
		yield();
	}
	APP_SERIAL_DEBUG("\nclearFlash done\n");
}

/*
 * performs a SPIFFS.format() operation on the SPI file system flash area
 */
void FlashAppDataManager::formatSPIFFS() {

	if (!SPIFFS.format()) {
		APP_SERIAL_DEBUG("SPIFFS.format() failed\n");
	}
    else {
		APP_SERIAL_DEBUG("SPIFFS.format() done\n");
    }

}

