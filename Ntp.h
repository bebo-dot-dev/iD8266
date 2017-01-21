/*
 * ntp.h
 *
 *  Created on: 8 Mar 2016
 *      Author: joe
 */

#ifndef NTP_H_
#define NTP_H_

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Time.h"
#include "utils.h"

#define NTP_PACKET_SIZE 48 // NTP time is in the first 48 bytes of message
#define UDP_PORT 2390  // local port to listen for UDP packets

struct deviceUptime {
	long days;
	long hours;
	long mins;
	long secs;
};

class Ntp {
public:

	ICACHE_FLASH_ATTR Ntp(uint8_t server, int8_t tz, time_t syncSecs);
	ICACHE_FLASH_ATTR virtual ~Ntp();

	time_t lastTime = 0;
	static String ntpServerName;
	static String fallbackNtpServerName;
	static int8_t timezone;
	static time_t syncInterval;
	static bool syncResponseReceived;
	static time_t lastSync;
	static uint16_t lastSyncRetries;
	static IPAddress lastSyncIPAddress;

	static byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

	static WiFiUDP udpListener;

	static ICACHE_FLASH_ATTR String iso8601DateTime();
	static ICACHE_FLASH_ATTR deviceUptime getDeviceUptime();
	static ICACHE_FLASH_ATTR String getDeviceUptimeString();
	static ICACHE_FLASH_ATTR time_t getUtcTimeNow();
	void ICACHE_FLASH_ATTR processTime();

private:
	String ICACHE_FLASH_ATTR getNtpServerName(uint8_t idx);
	static ICACHE_FLASH_ATTR String zeroPaddedIntVal(int val);
	static ICACHE_FLASH_ATTR time_t getNtpTime();
	static ICACHE_FLASH_ATTR void sendNTPpacket(IPAddress &ntpServerIpAddr);
};

#endif /* NTP_H_ */
