/*
 * gpioManager.h
 *
 *  Created on: 17 Mar 2016
 *      Author: joe
 */

#ifndef GPIOMANAGER_H_
#define GPIOMANAGER_H_

#include <Arduino.h>

#define MAX_GPIO 5
#define UNDEFINED_GPIO -32768
#define MIN_ANALOG 0
#define MAX_ANALOG 255
#define MAX_NAME_STR 32
#define MAX_ANALOG_READINGS 64

enum gpioType
{
	digital = 0,
	analog = 1
};

enum digitalMode
{
	digitalNotInUse = 0, //"pin not in use / not initialised"
	digitalInput = 1, //"pin is simple digital INPUT (0x00 and read with digitalRead)"
	digitalInputPullup = 2, //"pin is simple digital INPUT (0x00 and read with digitalRead)"
	digitalOutput = 3, //"pin is simple digital OUTPUT (0x01 and written with digitalWrite)"
	digitalAnalogOutputPwm = 4 //"pin is PWM OUTPUT (0x01 and written with analogWrite)"
};

enum analogMode
{
	analogNotInUse = 0, //"pin not in use / not initialised"
	analogEnabled = 1, //"pin is enabled as analog INPUT (0x00 and read with analogRead)"
};

enum unitOfMeasure
{
	none = 0,
	celcius = 1,
	fahrenheit = 2,
	millimeter = 3,
	centimeter = 4,
	metre = 5,
	kilometer = 6,
	inch = 7,
	foot = 8,
	mile = 9,
	millilitre = 10,
	centilitre = 11,
	litre = 12,
	gallon = 13,
	milligram = 14,
	gram = 15,
	kilogram = 16,
	ounce = 17,
	pound = 18,
	ton = 19,
	btu = 20,
	millivolt = 21,
	volt = 22,
	milliampere = 23,
	ampere = 24,
	ohm = 25,
	watt = 26,
	unitOfMeasureMax = watt
};

struct digitalData {

	char digitalName[MAX_NAME_STR];
	digitalMode digitalPinMode;
	int defaultValue;
	int lastValue;
	bool logToThingSpeak;
	unsigned long thingSpeakChannel;
	char thingSpeakApiKey[MAX_NAME_STR];
	bool httpLoggingEnabled;

}  __attribute__ ((__packed__));

struct gpioStorage {

	//11 digital pins are available on an ESP12 however only 'safe' GPIO are surfaced
	//by this project, those defined by Kolban as low risk (page 125 of the book)
	digitalData digitalIO[MAX_GPIO];

	//1 analog pin is available on an ESP12
	char analogName[MAX_NAME_STR];
	analogMode analogPinMode;
	int analogRawValue; //raw analog value between 0 and 1024 on the ESP8266
	double analogVoltage; //between 0 and 1v on the ESP8266
	double analogOffset; //offset for calculated value
	double analogMultiplier; //multiplier for calculated value
	double analogCalcVal; //resulting calculated value
	unitOfMeasure analogUnit;
	bool stabilizationEnabled; //stabilize the raw analog value via array value averaging
	bool analogLogToThingSpeak;
	unsigned long analogThingSpeakChannel;
	char analogThingSpeakApiKey[MAX_NAME_STR];
	bool analogHttpLoggingEnabled;

} __attribute__ ((__packed__));

class GPIOManager {
public:
	ICACHE_FLASH_ATTR GPIOManager();
	virtual ICACHE_FLASH_ATTR ~GPIOManager();

	static const uint8_t digitalPinMap[MAX_GPIO];

	bool initialized;
	unsigned long lastInputRead;

	void ICACHE_FLASH_ATTR initialize();
	int ICACHE_FLASH_ATTR gpioDigitalRead(uint8_t pin);
	int ICACHE_FLASH_ATTR gpioAnalogRead();
	bool ICACHE_FLASH_ATTR gpioDigitalWrite(uint8_t pin, uint8_t value, bool suppressMqtt = false);
	bool ICACHE_FLASH_ATTR gpioAnalogWrite(uint8_t pin, int value, bool suppressMqtt = false);
	void ICACHE_FLASH_ATTR readInputs();
	static String ICACHE_FLASH_ATTR getUnitOfMeasureStr(unitOfMeasure unit);
private:
	int analogReadIndex = 0;
	bool stabilizationArrayFull = false;
	int analogRunningTotal = 0;
	int analogStabilizationArray[MAX_ANALOG_READINGS];
};

extern GPIOManager GPIOMngr;

#endif /* GPIOMANAGER_H_ */
