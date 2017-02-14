/*
 * DigistatMk2433.h
 *
 *  Created on: 1 Jan 2017
 *      Author: joe
 */

#ifndef DIGISTATMK2433_H_
#define DIGISTATMK2433_H_

#include <Arduino.h>
#include "GPIOData.h"
#include "interrupts.h"

#define DIGISTATMK2_433_MAX_WRITES 3
#define DIGISTATMK2_433_WRITE_REPEAT_INTERVAL 1900

/*
 * The class models emulation of a Digistat MK2 thermostat to enable control of a Worcester-Bosch Greenstar 30CDi combi-boiler
 * via a 433MHz transmitter connected to a regular GPIO pin configured as an output
 * Output pulses are sent as required to simulate the Digistat MK2 thermostat for boiler on/off control
 * Actual 433MHz pulse streams were originally sniffed over the air with an SDR dongle @433.92MHz
 */
class DigistatMk2_433 {
private:
	uint8_t mappedPin;
	peripheralData *_peripheralData;
	uint8_t writeCounter;
	unsigned long lastWrite;
	static const uint16_t preambleSection[14];
	static const uint16_t boilerIdSection[16];
	static const uint16_t offSignalSection[30];
	static const uint16_t onSignalSection[28];
	void ICACHE_FLASH_ATTR SwitchOff();
	void ICACHE_FLASH_ATTR SwitchOn();
	bool ICACHE_FLASH_ATTR TimeToRepeat();
public:
	ICACHE_FLASH_ATTR DigistatMk2_433(peripheralData *data);
	void ICACHE_FLASH_ATTR Switch(uint8_t value);
	void ICACHE_FLASH_ATTR Repeat();
};

#endif /* DIGISTATMK2433_H_ */
