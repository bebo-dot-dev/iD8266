/*
 * PowerManager.cpp
 *
 *  Created on: 6 Jul 2016
 *      Author: joe
 */

#include "PowerManager.h"
#include "flashAppData.h"
#include "NetworkServices.h"
#include "applicationStrings.h"

//the global device wide rtcConfigData rtcData
struct rtcData rtcConfigData;

ICACHE_FLASH_ATTR PowerManager::PowerManager(rst_info *resetInfo) {

	isDisabled = false;
	delayedSleep = false;
	wakeupTime = millis();
	wakeTimestamp = 0; //is set in logBoot when ntp has started

	resetReason = (rst_reason)resetInfo->reason;

	wakeEventType =
		resetInfo->reason == rst_reason::REASON_DEFAULT_RST //power on boot / CH_PD->ground
		? powerEventType::manualWakeEvent
		: powerEventType::automatedWakeEvent;

	APP_SERIAL_DEBUG((getEventTypeString(wakeEventType) + getAppStr(appStrType::newline)).c_str());

	if (wakeEventType == powerEventType::manualWakeEvent) {
		delaySleep();
	}

	if ((appConfigData.powerMgmt.enabled) && (wakeEventType == powerEventType::automatedWakeEvent)) {
		checkLengthySleep();
	} else {
		initRtcStoreData();
	}

	if (resetReason != rst_reason::REASON_DEEP_SLEEP_AWAKE) {
		rtcConfigData.lastSleepTimestamp = 0;
	}

}

/*
 * reads RTC memory to determine if a lengthy sleep interval has been initiated / is in progress
 */
void ICACHE_FLASH_ATTR PowerManager::checkLengthySleep() {

	if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcConfigData, sizeof(rtcData))) {

		if(rtcConfigData.targetSleepCount > 0) {

			//uint32_t crcCalc = calculateCRC32(((uint8_t*) &rtcConfigData) + 4, sizeof(rtcData) - 4);

			//APP_SERIAL_DEBUG("CRC32 of data: %s \n", String(crcCalc, HEX).c_str());
			//APP_SERIAL_DEBUG("CRC32 read from RTC: %s \n", String(rtcStore.crc32, HEX).c_str());

			//if (crcCalc == rtcConfigData.crc32) {
			//RTC data looks good
			rtcConfigData.targetSleepCount--;

			if (rtcConfigData.targetSleepCount == 0) {

				if (wakeEventType == powerEventType::automatedWakeEvent) {
					wakeEventType = powerEventType::scheduledWakeEvent;
				}
				return; //the target number of sleeps has been reached so just continue normal processing
			}

			//a lengthy sleep is in progress - immediately go back to sleep
			if (rtcConfigData.targetSleepCount == 1) {
				rtcConfigData.wakeMode = WakeMode::RF_DEFAULT; // next wake up has WiFi on
				deepSleep(false);
			} else {
				rtcConfigData.wakeMode = WakeMode::RF_DISABLED;
				deepSleep(false);
			}
			//}
		}
	}
}

bool ICACHE_FLASH_ATTR PowerManager::isSingleCycleEnabled() {
	return appConfigData.powerMgmt.enabled &&
			appConfigData.powerMgmt.onLength == powerOnLength::zeroLength &&
			!delayedSleep;
}

bool ICACHE_FLASH_ATTR PowerManager::addSchedule(uint8_t day, uint8_t hr, powerDownLength offLength) {

	bool scheduleAdded = false;
	scheduleData *schedule = NULL;

	for(byte i = 0; i < MAX_SCHEDULES; i++) {
		schedule = &appConfigData.powerMgmt.schedules[i];
		if (!schedule->enabled) {

			schedule->weekday = day;
			schedule->hour = hr;
			schedule->offLength = offLength;
			schedule->enabled = true;

			scheduleAdded = true;

			if ((schedule->weekday == weekday()) && (schedule->hour == hour())) {
				//the schedule being added matches now() so switch on the 5 minute sleep delay so we don't immediately shutdown
				NetworkSvcMngr.powerManager->delaySleep();
			}

			break;
		}
	}

	return scheduleAdded;
}

bool ICACHE_FLASH_ATTR PowerManager::removeSchedule(uint8_t scheduleIdx) {

	bool scheduleRemoved = false;

	if (scheduleIdx < MAX_SCHEDULES) {

		scheduleData *schedule = &appConfigData.powerMgmt.schedules[scheduleIdx];
		schedule->enabled = false;
		schedule->hour = 0;
		schedule->weekday = 0;
		schedule->offLength = powerDownLength::eightHours;

		scheduleRemoved = true;

	}

	return scheduleRemoved;
}

void ICACHE_FLASH_ATTR PowerManager::delaySleep() {
	delayedSleep = true;
	wakeupTime = millis();
}

void ICACHE_FLASH_ATTR PowerManager::deepSleep(bool sleepLoggingEnabled) {

	if (sleepLoggingEnabled) {
		//TODO: mqtt support for power down event
	}

	APP_SERIAL_DEBUG((getEventTypeString(rtcConfigData.eventType) + getAppStr(appStrType::newline)).c_str());

	if ((appConfigData.ntpEnabled) && (NetworkSvcMngr.ntpManager != NULL) && (NetworkSvcMngr.ntpManager->syncResponseReceived)) {
		rtcConfigData.lastSleepTimestamp = now();
		APP_SERIAL_DEBUG("Timestamp deep sleep: %d\n", (int)rtcConfigData.lastSleepTimestamp);
	} else {
		rtcConfigData.lastSleepTimestamp = 0;
	}

	//rtcConfigData.crc32 = calculateCRC32(((uint8_t*) &rtcConfigData) + 4, sizeof(rtcData) - 4);

	//persist current config to RTC memory
	ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcConfigData, sizeof(rtcData));

	APP_SERIAL_DEBUG("Going into deep sleep for %d microseconds now", rtcConfigData.pwrDownLength);

	//initiate sleep for the specified sleepLength
	ESP.deepSleep(rtcConfigData.pwrDownLength, rtcConfigData.wakeMode);
}

scheduleData* ICACHE_FLASH_ATTR PowerManager::getScheduledPowerDownData(int hour) {

	scheduleData *schedule = NULL;
	scheduleData *sched;
	bool wakeExpired = true;
	if (delayedSleep) {
		//delay sleep for 5 mins
		wakeExpired = (millis() >= wakeupTime + FIVE_MIN_MILLIS);
	}

	if (wakeExpired) {
		for(byte i = 0; i < MAX_SCHEDULES; i++) {
			sched = &appConfigData.powerMgmt.schedules[i];
			//try and find an enabled schedule that matches the current NTP day and hour
			if ((sched->enabled) && (sched->weekday == weekday()) && (sched->hour == hour)) {
				schedule = sched;
				break;
			}
		}
	}

	return schedule;
}

bool ICACHE_FLASH_ATTR PowerManager::powerOnLengthExpired() {

	bool expired = false;
	unsigned long now = millis();

	switch(appConfigData.powerMgmt.onLength) {

		case powerOnLength::infiniteLength:
			expired = false;
			break;
		case powerOnLength::oneMinute:
			expired = (now >= wakeupTime + ONE_MIN_MILLIS);
			break;
		case powerOnLength::twoMinutes:
			expired = (now >= wakeupTime + TWO_MIN_MILLIS);
			break;
		case powerOnLength::fiveMinutes:
			expired = (now >= wakeupTime + FIVE_MIN_MILLIS);
			break;
		case powerOnLength::fifteenMinutes:
			expired = (now >= wakeupTime + FIFTEEN_MIN_MILLIS);
			break;
		default:
			//powerOnLength::zeroLength
			expired = true;

	}

	if (delayedSleep) {
		//delay sleep for 5 mins
		expired &= (now >= wakeupTime + FIVE_MIN_MILLIS);
	}


	return expired;
}

void ICACHE_FLASH_ATTR PowerManager::initRtcStoreData() {

	//rtcConfigData.crc32 = 0;
	rtcConfigData.targetSleepCount = 0;
	rtcConfigData.pwrDownLength = 0;
	rtcConfigData.eventType = powerEventType::automatedWakeEvent;
	rtcConfigData.wakeMode = WakeMode::RF_DEFAULT;
	rtcConfigData.lastSleepTimestamp = 0;
	rtcConfigData.lastSleepAdjusted = false;
	rtcConfigData.shortSleep = false;

	ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcConfigData, sizeof(rtcData));

}

/*
 * calculates a thirty minute interval to land exactly on the hour or half past the hour when waking from schedule and ntp is enabled
 * otherwise simply returns THIRTY_MIN_MICROS
 */
uint32_t ICACHE_FLASH_ATTR PowerManager::getThirtyMinuteInterval(bool ntpAdjust) {

	uint32_t interval = THIRTY_MIN_MICROS;

	if (ntpAdjust && wakeEventType == powerEventType::scheduledWakeEvent && appConfigData.ntpEnabled && NetworkSvcMngr.ntpManager->syncResponseReceived) {

		time_t timenow = now();
		time_t fromnow = (timenow + THIRTY_MIN_SECONDS);

		tmElements_t te;
		breakTime(fromnow, te);

		te.Second = 0;

		if (te.Minute >= 53 && te.Minute <= 7) {
			te.Minute = 0;
		} else
			if (te.Minute >= 8 && te.Minute <= 22) {
				te.Minute = 15;
			} else
				if (te.Minute >= 23 && te.Minute <= 37) {
					te.Minute = 30;
				} else
					if (te.Minute >= 38 && te.Minute <= 52) {
						te.Minute = 45;
					}

		fromnow = makeTime(te);
		long diffSecs = (fromnow - timenow);

		//convert the interval in seconds to microseconds
		interval = (diffSecs * 1000) * 1000;

	}

	return interval;
}

/*
 * calculates a one hour interval to land exactly on the hour when waking from schedule and ntp is enabled
 * otherwise simply returns ONE_HOUR_MICROS
 */
uint32_t ICACHE_FLASH_ATTR PowerManager::getOneHourInterval(bool ntpAdjust) {

	uint32_t interval = ONE_HOUR_MICROS;

	if (ntpAdjust && wakeEventType == powerEventType::scheduledWakeEvent && appConfigData.ntpEnabled && NetworkSvcMngr.ntpManager->syncResponseReceived) {

		time_t timenow = now();
		time_t fromnow = (timenow + ONE_HOUR_SECONDS);

		tmElements_t te;
		breakTime(fromnow, te);

		te.Second = 0;

		if (te.Minute > 30) {

			fromnow = (timenow + ONE_HOUR_SECONDS);
			breakTime(fromnow, te);
			te.Second = 0;
		}

		te.Minute = 0;

		fromnow = makeTime(te);
		long diffSecs = (fromnow - timenow);

		//convert the interval in seconds to microseconds
		interval = (diffSecs * 1000) * 1000;

	}

	return interval;
}

/*
 * adjusts the configured rtcConfigData.pwrDownLength by considering any drift detected in the interval
 * between the wake time and the last sleep timestamp
 */
void ICACHE_FLASH_ATTR PowerManager::adjustPowerDownLength() {

	if ((wakeTimestamp != 0) &&
		(rtcConfigData.lastSleepTimestamp != 0) &&
		(!rtcConfigData.lastSleepAdjusted)) {

		APP_SERIAL_DEBUG("Adjusting power down length: %d\n", rtcConfigData.pwrDownLength);
		float lastSleepIntervalSecs = (float)(wakeTimestamp - rtcConfigData.lastSleepTimestamp);
		APP_SERIAL_DEBUG("Last sleep interval in seconds: %d\n", (int)lastSleepIntervalSecs);

		float adjustmentFactor = ((rtcConfigData.pwrDownLength / 1000) / 1000) / lastSleepIntervalSecs;
		rtcConfigData.pwrDownLength = (uint32_t)(rtcConfigData.pwrDownLength * adjustmentFactor);
		rtcConfigData.lastSleepAdjusted = true;

		APP_SERIAL_DEBUG("Adjusted power down length: %d\n", rtcConfigData.pwrDownLength);
	} else {
		rtcConfigData.lastSleepAdjusted = false;
	}
}

/*
 * returns a sleep interval in microseconds that corresponds to the given powerDownLength
 */
uint32_t ICACHE_FLASH_ATTR PowerManager::getSleepInterval(powerDownLength length, bool ntpAdjust) {

	switch(length) {
		case powerDownLength::fiveMins:
			return FIVE_MIN_MICROS;
		case powerDownLength::thirtyMinutes:
			return getThirtyMinuteInterval(ntpAdjust);
		default:
			return getOneHourInterval(ntpAdjust);
	}
}

/*
 * sets up the rctStore rtcData correctly for a sleep interval of the given powerDownLength
 */
void ICACHE_FLASH_ATTR PowerManager::getRtcStoreData(powerDownLength length, powerEventType eventType) {

	rtcConfigData.targetSleepCount = 0; //default to no lengthy sleep interval
	rtcConfigData.pwrDownLength = getSleepInterval(length);
	rtcConfigData.eventType = eventType;
	rtcConfigData.wakeMode = WakeMode::RF_DISABLED; //default to radio off on wake
	rtcConfigData.shortSleep = false;

	if (!checkShortSleep()) {

		switch(length) {

			case powerDownLength::fiveMins:
			case powerDownLength::thirtyMinutes:
			case powerDownLength::oneHour:
				rtcConfigData.wakeMode = WakeMode::RF_DEFAULT;
				break;
			case powerDownLength::threeHours:
				rtcConfigData.targetSleepCount = 3;
				break;
			case powerDownLength::sixHours:
				rtcConfigData.targetSleepCount = 6;
				break;
			case powerDownLength::eightHours:
				rtcConfigData.targetSleepCount = 8;
				break;
			case powerDownLength::twelveHours:
				rtcConfigData.targetSleepCount = 12;
				break;
			case powerDownLength::sixteenHours:
				rtcConfigData.targetSleepCount = 16;
				break;
			case powerDownLength::twentyHours:
				rtcConfigData.targetSleepCount = 20;
				break;
			case powerDownLength::twentyFourHours:
				rtcConfigData.targetSleepCount = 24;
				break;

		}

		adjustPowerDownLength();
	}
}

String ICACHE_FLASH_ATTR PowerManager::getEventTypeString(powerEventType eventType) {

	switch(eventType) {

		case powerEventType::automatedWakeEvent:
			return getAppStr(appStrType::automatedWake);

		case powerEventType::scheduledWakeEvent:
			return getAppStr(appStrType::scheduledWake);

		case powerEventType::automatedSleepEvent:
			return getAppStr(appStrType::automatedSleep);

		case powerEventType::scheduledSleepEvent:
			return getAppStr(appStrType::scheduledSleep);

		default:
			return getAppStr(appStrType::manualWake);

	}
}

uint32_t ICACHE_FLASH_ATTR PowerManager::calculateCRC32(const uint8_t *data, size_t length)
{
	uint32_t crc = 0xffffffff;
	while (length--) {
		uint8_t c = *data++;
		for (uint32_t i = 0x80; i > 0; i >>= 1) {
			bool bit = crc & 0x80000000;
			if (c & i) {
				bit = !bit;
			}
			crc <<= 1;
			if (bit) {
				crc ^= 0x04c11db7;
			}
		}
		yield();
	}
	return crc;
}

/*
 * checks if a shortened immediate sleep is required to reach the next scheduled sleep
 */
bool ICACHE_FLASH_ATTR PowerManager::checkShortSleep(bool canSleepNow) {

	if (isSingleCycleEnabled() && appConfigData.ntpEnabled && NetworkSvcMngr.ntpManager->syncResponseReceived) {

		uint32_t sleepInterval = getSleepInterval(appConfigData.powerMgmt.offLength, false);
		time_t timenow = now();
		time_t fromnow = (timenow + (sleepInterval / 1000));

		tmElements_t te;
		breakTime(fromnow, te);

		scheduleData* schedule = getScheduledPowerDownData(te.Hour);
		if (schedule != NULL && (hour() != schedule->hour)) {
			//we have an upcoming schedule so calculate the shortened immediate sleep interval
			//and either schedule the short sleep or immediately go to sleep
			te.Minute = 0;
			te.Second = 0;
			fromnow = makeTime(te);
			long diffSecs = (fromnow - timenow);

			rtcConfigData.wakeMode = RFMode::RF_DEFAULT;
			rtcConfigData.eventType = powerEventType::automatedSleepEvent;
			if (diffSecs > 300) {
				//sleep for a 5 minute chunk
				rtcConfigData.pwrDownLength = FIVE_MIN_MICROS;
			} else {
				//sleep for a 1 minute chunk
				rtcConfigData.pwrDownLength = ONE_MIN_MICROS;
			}

			if (!rtcConfigData.shortSleep || !canSleepNow) {
				//delayed short sleep
				rtcConfigData.shortSleep = true;
			} else {
				//short sleep now
				deepSleep(false);
			}

			return true;

		} else {
			rtcConfigData.shortSleep = false;
		}
	} else {
		rtcConfigData.shortSleep = false;
	}

	return false;
}

void ICACHE_FLASH_ATTR PowerManager::logBoot() {

	if ((appConfigData.ntpEnabled) && (NetworkSvcMngr.ntpManager->syncResponseReceived)) {
		unsigned long diffSecs = (millis() - wakeupTime) / 1000;
		wakeTimestamp = now() - diffSecs;
		checkShortSleep(true);
	} else {
		wakeTimestamp = 0;
	}

	//TODO: mqtt support for boot event
}

void ICACHE_FLASH_ATTR PowerManager::processPowerEvents() {

	if ((appConfigData.powerMgmt.enabled) && (!isDisabled)) {

		if ((appConfigData.ntpEnabled) && (NetworkSvcMngr.ntpManager->syncResponseReceived)) {

			scheduleData* schedule = getScheduledPowerDownData(hour());
			if (schedule != NULL) {

				//scheduled sleep initiation
				isDisabled = true;

				getRtcStoreData(schedule->offLength, powerEventType::scheduledSleepEvent);
				deepSleep();
			}
		}

		if ((!isDisabled) && (rtcConfigData.shortSleep)) {

			//automated short sleep leading up to the next scheduled sleep
			isDisabled = true;
			deepSleep();
		}

		if ((!isDisabled) && (powerOnLengthExpired())) {

			//automated sleep initiation
			isDisabled = true;

			getRtcStoreData(appConfigData.powerMgmt.offLength, powerEventType::automatedSleepEvent);
			deepSleep();

		}
	}
}
