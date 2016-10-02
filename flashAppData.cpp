/*
  flashappData.ino - exposes functions for reading/writing appData configuration from flash
  and also functions for managing and maintaining flash memory for the device
*/
#include <EEPROM.h>
#include <FS.h>
#include <Arduino.h>
extern "C" {
	#include <user_interface.h>
}
#include "utils.h"
#include "flashAppData.h"
#include "NetworkServices.h"
#include "PowerManager.h"

//the global device wide appConfigData appData
struct appData appConfigData;

const char jjs8266InitMarker[] = "JJS8266_INIT_V0";
const char* defaultDeviceNamePrefix = "iD8266_";
const char* defaultPwd = "password1234";
const char* defaultLocale = "en-GB";
const char* defaultAnalogName = "analogInput";

const size_t initMarkerSize = sizeof(jjs8266InitMarker);
const size_t appDataSize = sizeof(appData);
const size_t appFlashSize = (initMarkerSize + appDataSize);
const WiFiMode defaultWifiMode = WIFI_AP;


appData getAppData() {

  EEPROM.begin(appFlashSize);
  appConfigData = EEPROM.get(initMarkerSize, appConfigData);
  return appConfigData;

}

void setAppData() {

  appConfigData.initialized = true;
  EEPROM.begin(appFlashSize);
  appConfigData = EEPROM.put(initMarkerSize, appConfigData);
  EEPROM.end();

}

//track unnatural system reset events
void trackSystemResetEvents() {

	if(appConfigData.initialized) {

		rst_info *resetInfo = ESP.getResetInfoPtr();

		switch(resetInfo->reason)
		{
			case rst_reason::REASON_WDT_RST :
				appConfigData.wdtResetCount++;
				setAppData();
				break;
			case rst_reason::REASON_EXCEPTION_RST :
				appConfigData.exceptionResetCount++;
				setAppData();
				break;
			case rst_reason::REASON_SOFT_WDT_RST :
				appConfigData.softWdtResetCount++;
				setAppData();
				break;
			default:
				break;
		}
	}
}

appData initFlash(bool forceInit, bool formatFileSystem, bool trackSystemReset) {

	APP_SERIAL_DEBUG("\ninitFlash start\n");

	EEPROM.begin(initMarkerSize);
	char initMarker[initMarkerSize];
	EEPROM.get(0, initMarker);
	EEPROM.end();

	APP_SERIAL_DEBUG("Flash init marker: %s\n", initMarker);

	if ((strcmp(initMarker, jjs8266InitMarker) != 0) || (forceInit)) {

		APP_SERIAL_DEBUG("Initialising flash\n");

		clearFlash();

		if(formatFileSystem)
		  formatSPIFFS();

		EEPROM.begin(initMarkerSize);
		EEPROM.put(0, jjs8266InitMarker);
		EEPROM.end();

		appConfigData.wifiMode = defaultWifiMode;

		String deviceName = defaultDeviceNamePrefix;
		deviceName += NetworkServiceManager.devMacLastSix();

		strncpy(appConfigData.deviceApSSID, deviceName.c_str(), STRMAX);
		strncpy(appConfigData.deviceApPwd, defaultPwd, STRMAX);
		appConfigData.deviceApChannel = 1;

		strncpy(appConfigData.networkApSSID, EMPTY_STR, STRMAX);
		strncpy(appConfigData.networkApPwd, EMPTY_STR, STRMAX);
		appConfigData.networkApIpMode = Dhcp;

		appConfigData.webserverPort = 80;
		strncpy(appConfigData.adminPwd, defaultPwd, STRMAX);
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
		appConfigData.ntpSyncInterval = (60 * 60 * 12); //default sync is every 12 hours
		strncpy(appConfigData.ntpLocale, defaultLocale, STRLOCALE);

		appConfigData.wdtResetCount = 0;
		appConfigData.exceptionResetCount = 0;
		appConfigData.softWdtResetCount = 0;

		String digitalName;
		for(int i = 0; i < MAX_GPIO; i++) {
			digitalName = "gpio_" + String(i);
			strncpy(appConfigData.gpio.digitalIO[i].digitalName, digitalName.c_str(), STRMAX);
			appConfigData.gpio.digitalIO[i].digitalPinMode = digitalNotInUse;
			appConfigData.gpio.digitalIO[i].defaultValue = 0;
			appConfigData.gpio.digitalIO[i].lastValue = UNDEFINED_GPIO;

			appConfigData.gpio.digitalIO[i].logToThingSpeak = false;
			appConfigData.gpio.digitalIO[i].thingSpeakChannel = 0;
			strncpy(appConfigData.gpio.digitalIO[i].thingSpeakApiKey, EMPTY_STR, STRMAX);
			appConfigData.gpio.digitalIO[i].httpLoggingEnabled = false;
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

		appConfigData.gpio.analogLogToThingSpeak = false;
		appConfigData.gpio.analogThingSpeakChannel = 0;
		strncpy(appConfigData.gpio.analogThingSpeakApiKey, EMPTY_STR, STRMAX);
		appConfigData.gpio.analogHttpLoggingEnabled = false;

		appConfigData.thingSpeakEnabled = false;

		appConfigData.httpLoggingEnabled = false;
		strncpy(appConfigData.httpLoggingHost, EMPTY_STR, STRMAX);
		strncpy(appConfigData.httpLoggingUri, EMPTY_STR, LARGESTRMAX);
		//(appConfigData.httpLoggingHostSslFingerprint, EMPTY_STR, DOUBLESTRMAX); //not enough memory to support SSL

		appConfigData.powerMgmt.enabled = false;
		appConfigData.powerMgmt.onLength = powerOnLength::infiniteLength;
		appConfigData.powerMgmt.offLength = powerDownLength::oneHour;

		appConfigData.powerMgmt.logToThingSpeak = false;
		appConfigData.powerMgmt.thingSpeakChannel = 0;
		strncpy(appConfigData.powerMgmt.thingSpeakApiKey, EMPTY_STR, STRMAX);
		appConfigData.powerMgmt.httpLoggingEnabled = false;
		strncpy(appConfigData.powerMgmt.httpLoggingHost, EMPTY_STR, STRMAX);
		strncpy(appConfigData.powerMgmt.httpLoggingUri, EMPTY_STR, LARGESTRMAX);

		for(int i = 0; i < MAX_SCHEDULES; i++) {
			appConfigData.powerMgmt.schedules[i].enabled = false;
			appConfigData.powerMgmt.schedules[i].weekday = 2;
			appConfigData.powerMgmt.schedules[i].hour = 0;
			appConfigData.powerMgmt.schedules[i].offLength = powerDownLength::eightHours;
		}

		setAppData();

		PowerManager::initRtcStoreData();

		APP_SERIAL_DEBUG("Initialising flash done\n");

	}
	else
	{
		APP_SERIAL_DEBUG("Flash is clean and initialised\n");
		getAppData();
	}

	if(trackSystemReset) {
		trackSystemResetEvents();
	}

	return appConfigData;
}

void clearFlash() {

  EEPROM.begin(appFlashSize);

  for (size_t i = 0 ; i < appFlashSize ; i++) {
    EEPROM.write(i, 0);
  }

  EEPROM.end();
}

void formatSPIFFS(){
  if (!SPIFFS.format()) {
      APP_SERIAL_DEBUG("SPIFFS.format() failed\n");
    }
    else {
      APP_SERIAL_DEBUG("SPIFFS.format() done\n");
    }
}

