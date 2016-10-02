/*
 * PowerManager.h
 *
 *  Created on: 6 Jul 2016
 *      Author: joe
 */

#ifndef POWERMANAGER_H_
#define POWERMANAGER_H_

extern "C" {
	#include <user_interface.h>
}
#include <Arduino.h>
#include "WiFiClient.h"
#include "/home/joe/git/Time/Time.h"

#define MAX_SCHEDULES 14
#define MAX_NAME_STR 32
#define LARGE_STRMAX 128

//time length constants
const uint32_t ONE_MIN_SECONDS = 60.0;
const uint32_t FIVE_MIN_SECONDS = (ONE_MIN_SECONDS * 5.0);
const uint32_t THIRTY_MIN_SECONDS = (ONE_MIN_SECONDS * 30.0);
const uint32_t ONE_HOUR_SECONDS = (ONE_MIN_SECONDS * 60.0);
const uint32_t ONE_MIN_MILLIS = (1000 * 60);
const uint32_t TWO_MIN_MILLIS = (ONE_MIN_MILLIS * 2);
const uint32_t FIVE_MIN_MILLIS = (ONE_MIN_MILLIS * 5);
const uint32_t FIFTEEN_MIN_MILLIS = (ONE_MIN_MILLIS * 15);
const uint32_t ONE_HOUR_MILLIS = (ONE_MIN_MILLIS * 60);
const uint32_t FIVE_MIN_MICROS = ((ONE_MIN_MILLIS * 5) * 1000);
const uint32_t THIRTY_MIN_MICROS = ((ONE_MIN_MILLIS * 30) * 1000);
const uint32_t ONE_HOUR_MICROS = ((ONE_MIN_MILLIS * 60) * 1000);

enum powerEventType {
	automatedWakeEvent = 0,
	manualWakeEvent = 1,
	scheduledWakeEvent = 2,
	automatedSleepEvent = 3,
	scheduledSleepEvent = 4
};

enum powerDownLength {
	fiveMins = 0,
	thirtyMinutes = 1,
	oneHour = 2,
	threeHours = 3,
	sixHours = 4,
	eightHours = 5,
	twelveHours = 6,
	sixteenHours = 7,
	twentyHours = 8,
	twentyFourHours = 9,
	maxPowerDownLength = twentyFourHours
};

enum powerOnLength {
	infiniteLength = 0, //no shutdown
	zeroLength = 1, //instant shutdown after first cycle
	oneMinute = 2,
	twoMinutes = 3,
	fiveMinutes = 4,
	fifteenMinutes = 5,
	maxPowerOnLength = fifteenMinutes
};

struct scheduleData {

	bool enabled;						//schedule on/off switch
	uint8_t weekday;					//the schedule day, Sunday is day 1
	uint8_t hour;						//the schedule hour
	powerDownLength offLength;			//describes how long to sleep for

} __attribute__ ((__packed__));

struct powerMgmtData {

	bool enabled;						//global on/off switch
	powerOnLength onLength;				//describes how long to stay awake when woken / powered on
	powerDownLength offLength;			//describes how long to sleep for
	//logging configuration
	bool httpLoggingEnabled;
	char httpLoggingHost[MAX_NAME_STR];
	char httpLoggingUri[LARGE_STRMAX];
	bool logToThingSpeak;
	unsigned long thingSpeakChannel;
	char thingSpeakApiKey[MAX_NAME_STR];
	//automated long term scheduled power down data
	scheduleData schedules[MAX_SCHEDULES];

} __attribute__ ((__packed__));

struct rtcData {
	//uint32_t crc32;
	bool initialized;
	byte sleepCount;
	byte targetSleepCount;
	uint32_t pwrDownLength;
	powerEventType eventType;
	WakeMode wakeMode;
	time_t lastSleepTimestamp;
	bool lastSleepAdjusted;
};

class PowerManager {
public:
	bool isDisabled;			//toggle on/off switch flag
	bool delayedSleep;
	ICACHE_FLASH_ATTR PowerManager(rst_info *resetInfo);
	bool ICACHE_FLASH_ATTR addSchedule(uint8_t weekday, uint8_t hour, powerDownLength offLength);
	bool ICACHE_FLASH_ATTR removeSchedule(uint8_t scheduleIdx);
	void ICACHE_FLASH_ATTR delaySleep();
	void ICACHE_FLASH_ATTR logBoot();
	void ICACHE_FLASH_ATTR processPowerEvents();
	static void ICACHE_FLASH_ATTR initRtcStoreData();
private:
	rst_reason resetReason;
	powerEventType wakeEventType;
	unsigned long wakeupTime;	//storage for millis() at wake up / power on time
	time_t wakeTimestamp;  //storage for now() at wake up / power on time
	unsigned long lastThingSpeakLog;
	WiFiClient* pwrMgmtWifiClient;
	WiFiClient* ICACHE_FLASH_ATTR getWifiClient();
	scheduleData* ICACHE_FLASH_ATTR getScheduledPowerDownData();
	bool ICACHE_FLASH_ATTR powerOnLengthExpired();
	void ICACHE_FLASH_ATTR getRtcStoreData(powerDownLength length, powerEventType eventType);
	String ICACHE_FLASH_ATTR getEventTypeString(powerEventType eventType);
	void ICACHE_FLASH_ATTR deepSleep(bool sleepLoggingEnabled = true);
	int ICACHE_FLASH_ATTR logToThingSpeak(powerEventType eventType);
	bool ICACHE_FLASH_ATTR logToHttpServer(powerEventType eventType);
	void ICACHE_FLASH_ATTR checkLengthySleep();
	uint32_t ICACHE_FLASH_ATTR calculateCRC32(const uint8_t *data, size_t length);
	void ICACHE_FLASH_ATTR adjustPowerDownLength();
	uint32_t ICACHE_FLASH_ATTR getThirtyMinuteInterval();
	uint32_t ICACHE_FLASH_ATTR getOneHourInterval();
};

extern struct rtcData rtcConfigData;

#endif /* POWERMANAGER_H_ */
