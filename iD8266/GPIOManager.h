/*
 * gpioManager.h
 *
 *  Created on: 17 Mar 2016
 *      Author: joe
 */

#ifndef GPIOMANAGER_H_
#define GPIOMANAGER_H_

#include <Arduino.h>
#include "DHT.h"
#include "GPIOData.h"
#include "DigistatMk2433.h"
#include "NexaCtrl.h"

class GPIOManager {
public:
	ICACHE_FLASH_ATTR GPIOManager();
	virtual ICACHE_FLASH_ATTR ~GPIOManager();

	static const uint8_t digitalPinMap[MAX_DEVICES];

	bool initialized;
	unsigned long lastProcess;
	unsigned long lastDHTRead;
	unsigned long lastHomeEasyBroadcast;

	void ICACHE_FLASH_ATTR initialize();

	bool ICACHE_FLASH_ATTR addPeripheral(peripheralType ptype, String &peripheralName, uint8_t pinIdx, int defaultValue, uint8_t virtualDeviceId);
	bool ICACHE_FLASH_ATTR removePeripheral(uint8_t deviceIdx);
	bool ICACHE_FLASH_ATTR removePeripheralsByPinIdx(uint8_t pinIdx);
	peripheralData ICACHE_FLASH_ATTR *getPeripheralByIdx(uint8_t deviceIdx);

	int ICACHE_FLASH_ATTR gpioDigitalRead(uint8_t pin);
	int ICACHE_FLASH_ATTR gpioAnalogRead();
	bool ICACHE_FLASH_ATTR gpioDigitalWrite(uint8_t pin, uint8_t value);
	bool ICACHE_FLASH_ATTR gpioAnalogWrite(uint8_t pin, int value);
	bool ICACHE_FLASH_ATTR peripheralWrite(uint8_t deviceIdx, peripheralType ptype, uint8_t virtualDeviceId, uint8_t value);
	void ICACHE_FLASH_ATTR dhtRead(uint8_t deviceIdx);
	void ICACHE_FLASH_ATTR processGPIO();
	static String ICACHE_FLASH_ATTR getUnitOfMeasureStr(unitOfMeasure unit);
private:
	int analogReadIndex = 0;
	bool stabilizationArrayFull = false;
	int analogRunningTotal = 0;
	int analogStabilizationArray[MAX_ANALOG_READINGS];
	DigistatMk2_433 *digistat;
	DHT *dht;
	NexaCtrl *homeEasyController;
};

extern GPIOManager GPIOMngr;

#endif /* GPIOMANAGER_H_ */
