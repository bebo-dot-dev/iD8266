/*
 * DigistatMk2433.cpp
 *
 *  Created on: 1 Jan 2017
 *      Author: joe
 */

#include "DigistatMk2433.h"
#include "utils.h"
#include "GPIOManager.h"

//                                                     on  off on  off on  off on  off on  off on  off on   off
const uint16_t DigistatMk2_433::preambleSection[14] = {400,460,505,495,505,485,505,465,505,485,505,475,1000,1000};
//                                                   on  off on  off on  off on  off  on  off on   off on  off on   off
const uint16_t DigistatMk2_433::boilerIdSection[16] {490,495,505,495,505,485,505,1000,505,485,1000,475,505,950,1050,1000};
//                                                    on  off on  off on  off on  off on  off on  off on  off on  off on  off on  off on   off  on  off on  off  on off  on   off
const uint16_t DigistatMk2_433::offSignalSection[30] {450,450,505,485,505,475,505,485,505,485,505,485,505,485,505,495,505,470,505,485,1000,1000,505,500,980,485,505,1000,1000,1};
//                                                   on  off on  off on  off on  off on  off on   off on  off on  off on  off on  off on   off  on  off on  off on   off
const uint16_t DigistatMk2_433::onSignalSection[28] {440,450,505,485,505,495,505,485,505,485,1000,970,505,485,505,495,505,485,505,485,1010,1060,950,485,505,980,1000,1};

/*
 * constructor
 */
ICACHE_FLASH_ATTR DigistatMk2_433::DigistatMk2_433(peripheralData *data) {

	_peripheralData = data;

	writeCounter = 0;
	lastWrite = millis();

	mappedPin = GPIOManager::digitalPinMap[_peripheralData->pinIdx];
	APP_SERIAL_DEBUG("Pin %d (idx:%d, '%s') is digitstatMk2 OUTPUT\n", mappedPin, _peripheralData->pinIdx, _peripheralData->base.name);

	//setup the mappedPin as an OUTPUT
	pinMode(mappedPin, OUTPUT);

}

/*
 * primary switch on/off public interface method
 */
void ICACHE_FLASH_ATTR DigistatMk2_433::Switch(uint8_t value) {

	APP_SERIAL_DEBUG("Writing digistatMk2 output value %d for pin %d (idx:%d, '%s')\n", value, mappedPin, _peripheralData->pinIdx, _peripheralData->base.name);

	writeCounter = 0;

	switch(value){
		case LOW:
			SwitchOff();
			break;
		case HIGH:
			SwitchOn();
			break;
	}

}

/*uint16_t ICACHE_FLASH_ATTR read_rom_uint16(const uint16_t* addr){
    uint32_t bytes;
    bytes = *(uint32_t*)((uint32_t)addr & ~3);
    return ((uint16_t*)&bytes)[((uint32_t)addr >> 1) & 1];
}*/

/*
 * 433MHz OFF pulse stream (sent 3 times with a ~2 second gap between iterations)
 */
void ICACHE_FLASH_ATTR DigistatMk2_433::SwitchOff() {

	uint8_t pinState = LOW;
	digitalWrite(mappedPin, pinState);
	delay(20);

	//timing critical start
	{
		InterruptLock lock;

		uint8_t loopSize = sizeof(DigistatMk2_433::preambleSection) / sizeof(uint16_t);
		for(uint8_t i = 0; i < loopSize; i++) {
				pinState = (pinState == LOW) ? HIGH : LOW;
				digitalWrite(mappedPin, pinState);
				uint16_t uS = DigistatMk2_433::preambleSection[i];
				delayMicroseconds(uS);
			}

		loopSize = sizeof(DigistatMk2_433::boilerIdSection) / sizeof(uint16_t);
		for(uint8_t i = 0; i < loopSize; i++) {
				pinState = (pinState == LOW) ? HIGH : LOW;
				digitalWrite(mappedPin, pinState);
				uint16_t uS = DigistatMk2_433::boilerIdSection[i];
				delayMicroseconds(uS);
			}

		loopSize = sizeof(DigistatMk2_433::offSignalSection) / sizeof(uint16_t);
		for(uint8_t i = 0; i < loopSize; i++) {
				pinState = (pinState == LOW) ? HIGH : LOW;
				digitalWrite(mappedPin, pinState);
				uint16_t uS = DigistatMk2_433::offSignalSection[i];
				delayMicroseconds(uS);
			}
	}
	//timing critical end

	yield();

	lastWrite = millis();
	writeCounter++;
	_peripheralData->base.lastValue = LOW;

}

/*
 * 433MHz ON pulse stream (sent 3 times with a ~2 second gap between iterations)
 */
void ICACHE_FLASH_ATTR DigistatMk2_433::SwitchOn() {

	uint8_t pinState = LOW;
	digitalWrite(mappedPin, pinState);
	delay(20);

	//timing critical start
	{
		InterruptLock lock;

		uint8_t loopSize = sizeof(DigistatMk2_433::preambleSection) / sizeof(uint16_t);
		for(uint8_t i = 0; i < loopSize; i++) {
				pinState = (pinState == LOW) ? HIGH : LOW;
				digitalWrite(mappedPin, pinState);
				uint16_t uS = DigistatMk2_433::preambleSection[i];
				delayMicroseconds(uS);
			}

		loopSize = sizeof(DigistatMk2_433::boilerIdSection) / sizeof(uint16_t);
		for(uint8_t i = 0; i < loopSize; i++) {
				pinState = (pinState == LOW) ? HIGH : LOW;
				digitalWrite(mappedPin, pinState);
				uint16_t uS = DigistatMk2_433::boilerIdSection[i];
				delayMicroseconds(uS);
			}

		loopSize = sizeof(DigistatMk2_433::onSignalSection) / sizeof(uint16_t);
		for(uint8_t i = 0; i < loopSize; i++) {
				pinState = (pinState == LOW) ? HIGH : LOW;
				digitalWrite(mappedPin, pinState);
				uint16_t uS = DigistatMk2_433::onSignalSection[i];
				delayMicroseconds(uS);
			}
	}
	//timing critical end

	yield();

	lastWrite = millis();
	writeCounter++;
	_peripheralData->base.lastValue = HIGH;
}

bool ICACHE_FLASH_ATTR DigistatMk2_433::TimeToRepeat() {

	bool repeat = false;
	unsigned long timeNow = millis();

	/*
	 * Implements an ON/OFF signal repeat function to prevent normal boiler failsafe switch off
	 * and boiler 'connection lost' scenarios occurring
	 * An ON/OFF signal repeat is sent every DIGISTATMK2_433_KEEP_ON_INTERVAL (60 seconds)
	 */
	if (((timeNow - lastWrite) > DIGISTATMK2_433_KEEP_ON_INTERVAL) || (timeNow < lastWrite)) {
		writeCounter = 0;
		repeat = true;
	}

	if (!repeat) {
		/*
		 * all ON / OFF signals are repeated 3 times on an approximate 2 second interval (DIGISTATMK2_433_WRITE_REPEAT_INTERVAL)
		 */
		if (((writeCounter < DIGISTATMK2_433_MAX_WRITES) && ((timeNow - lastWrite) > DIGISTATMK2_433_WRITE_REPEAT_INTERVAL)) || (timeNow < lastWrite)) {
			repeat = true;
		}
	}

	return repeat;
}


void ICACHE_FLASH_ATTR DigistatMk2_433::Repeat() {

	if (TimeToRepeat()) {

		switch(_peripheralData->base.lastValue){
			case LOW:
				SwitchOff();
				break;
			case HIGH:
				SwitchOn();
				break;
		}
	}
}



