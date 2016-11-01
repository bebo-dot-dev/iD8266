/*
 * ntp.cpp
 *
 *  Created on: 8 Mar 2016
 *      Author: joe
 */

#include "Ntp.h"
#include "applicationStrings.h"
#include "utils.h"
#include <WiFiUdp.h>

String Ntp::ntpServerName;
String Ntp::fallbackNtpServerName;
int8_t Ntp::timezone;
time_t Ntp::syncInterval;
bool Ntp::syncResponseReceived;
time_t Ntp::lastSync;
uint16_t Ntp::lastSyncRetries;
IPAddress Ntp::lastSyncIPAddress;

WiFiUDP Ntp::udpListener;
byte Ntp::packetBuffer[NTP_PACKET_SIZE];

ICACHE_FLASH_ATTR Ntp::Ntp(uint8_t server, int8_t tz, time_t syncSecs) {

	ntpServerName = getNtpServerName(server);
	fallbackNtpServerName = getNtpServerName(server + 1);
	timezone = tz;
	syncInterval = syncSecs;
	syncResponseReceived = false;
	lastSyncIPAddress = INADDR_NONE;

	udpListener.begin(UDP_PORT);

	setSyncProvider(getNtpTime);
	setSyncInterval(syncInterval);

}

ICACHE_FLASH_ATTR Ntp::~Ntp() {
	udpListener.stop();
}


String ICACHE_FLASH_ATTR Ntp::getNtpServerName(uint8_t idx) {

	switch(idx)
	{
		case 0:
			return getAppStr(appStrType::ntp1);
		case 1:
			return getAppStr(appStrType::ntp2);
		case 2:
			return getAppStr(appStrType::ntp3);
		case 3:
			return getAppStr(appStrType::ntp4);
		case 4:
			return getAppStr(appStrType::ntp5);
		case 5:
			return getAppStr(appStrType::ntp6);
		default:
			return getAppStr(appStrType::ntp1);
	}
}

time_t ICACHE_FLASH_ATTR Ntp::getNtpTime() {

	//ntp sync retry loop
	for(lastSyncRetries = 1; lastSyncRetries <= 5; lastSyncRetries++) {

		APP_SERIAL_DEBUG("NTP UDP flush\n");
		while (udpListener.parsePacket() > 0); // discard any previously received packets

		bool hasIP = false;

		String activeNtpServer = lastSyncRetries < 4 ? ntpServerName : fallbackNtpServerName;
		APP_SERIAL_DEBUG("NTP server hostname resolution for: %s\n", activeNtpServer.c_str());

		IPAddress timeServerIP;
		if (WiFi.hostByName(activeNtpServer.c_str(), timeServerIP)) {
			APP_SERIAL_DEBUG("NTP server hostname resolves to: %s\n", timeServerIP.toString().c_str());
			lastSyncIPAddress = timeServerIP;
			hasIP = true;
		} else {
			APP_SERIAL_DEBUG("NTP server hostname resolution failure\n");
		}

		if ((!hasIP) && (lastSyncIPAddress != INADDR_NONE)) {
			APP_SERIAL_DEBUG("Using IP address: %s from last successful DNS lookup\n", lastSyncIPAddress.toString().c_str());
			hasIP = true;
		}

		if (hasIP) {

			APP_SERIAL_DEBUG("Transmit NTP request attempt: %d\n", lastSyncRetries);
			sendNTPpacket(lastSyncIPAddress);

			uint32_t beginWait = millis();
			while (millis() - beginWait < 500) {
				int size = udpListener.parsePacket();
				if (size >= NTP_PACKET_SIZE) {
					APP_SERIAL_DEBUG("Received NTP response\n");
					udpListener.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
					unsigned long secsSince1900;
					// convert four bytes starting at location 40 to a long integer
					secsSince1900 = (unsigned long) packetBuffer[40] << 24;
					secsSince1900 |= (unsigned long) packetBuffer[41] << 16;
					secsSince1900 |= (unsigned long) packetBuffer[42] << 8;
					secsSince1900 |= (unsigned long) packetBuffer[43];
					syncResponseReceived = true;
					lastSync = secsSince1900 - 2208988800UL + timezone * SECS_PER_HOUR;
					return lastSync;
				}
				yield();
			}
		} else {
			APP_SERIAL_DEBUG("Skipped NTP request attempt: %d due to no NTP server IP being determined\n", lastSyncRetries);
		}
		delay(10);
	}

	APP_SERIAL_DEBUG("Get NTP time timeout\n");
	return 0; // return 0 if unable to get the time

}

// send an NTP request to the time server at the given address
void ICACHE_FLASH_ATTR Ntp::sendNTPpacket(IPAddress &ntpServerIpAddr) {
	syncResponseReceived = false;
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialise values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
	// all NTP fields have been given values..
	// send the packet requesting a timestamp:
	udpListener.beginPacket(ntpServerIpAddr, 123); //NTP requests are to port 123
	udpListener.write(packetBuffer, NTP_PACKET_SIZE);
	udpListener.endPacket();
}

void ICACHE_FLASH_ATTR Ntp::processTime() {

	timeStatus_t ts = timeStatus();

	if (ts == timeNeedsSync) {
		APP_SERIAL_DEBUG("Time needs sync\n");
	}

	switch (ts) {
	case timeNeedsSync:
	case timeSet:
		lastTime = now();
		break;
	default:
		break;
	}
}

String ICACHE_FLASH_ATTR Ntp::zeroPaddedIntVal(int val) {
	if (val < 10)
		return getAppStr(appStrType::zero) + String(val);
	else
		return String(val);
}

//returns the current date/time as a string in iso8601 format
String ICACHE_FLASH_ATTR Ntp::iso8601DateTime() {

	String hyphen = getAppStr(appStrType::hyphen);
	String colon = getAppStr(appStrType::colon);

	return	String(year()) + hyphen +
			zeroPaddedIntVal(month()) + hyphen +
			zeroPaddedIntVal(day()) + "T" +
			zeroPaddedIntVal(hour()) + colon +
			zeroPaddedIntVal(minute()) + colon +
			zeroPaddedIntVal(second()) +
			(timezone == 0 ? "Z" : String(timezone));
}

deviceUptime ICACHE_FLASH_ATTR Ntp::getDeviceUptime() {

	unsigned long currentmillis = millis();

	deviceUptime uptime;
	uptime.secs  = (long)((currentmillis / 1000) % 60);
	uptime.mins  = (long)((currentmillis / 60000) % 60);
	uptime.hours = (long)((currentmillis / 3600000) % 24);
	uptime.days  = (long)((currentmillis / 86400000) % 10);

	return uptime;

}

String ICACHE_FLASH_ATTR Ntp::getDeviceUptimeString() {

	deviceUptime uptime = getDeviceUptime();

	return	String(uptime.days) + getAppStr(appStrType::ntpDays) +
			String(uptime.hours) + getAppStr(appStrType::ntpHours) +
			String(uptime.mins) + getAppStr(appStrType::ntpMins) +
			String(uptime.secs) + getAppStr(appStrType::ntpSecs);

}
