/*
 * GPIOData.h
 *
 *  Created on: 5 Jan 2017
 *      Author: joe
 */

#ifndef GPIODATA_H_
#define GPIODATA_H_

#define MAX_DEVICES 5
#define UNDEFINED_GPIO -32768
#define UNDEFINED_GPIO_PIN 10
#define MIN_ANALOG 0
#define MAX_ANALOG 255
#define MAX_NAME_STR 32
#define MAX_ANALOG_READINGS 64
#define DEVICE_IO_PREFIX "gpio_"

enum ioType
{
	digital = 0,
	analog = 1
};

enum digitalMode
{
	digitalNotInUse = 0, //"pin not in use / not initialised"
	digitalInput = 1, //"pin is a simple digital INPUT (0x00 and read with digitalRead)"
	digitalInputPullup = 2, //"pin is a simple digital INPUT with internal pull up enabled (0x02 and read with digitalRead)"
	digitalOutput = 3, //"pin is a simple digital OUTPUT (0x01 and written with digitalWrite)"
	digitalAnalogOutputPwm = 4, //"pin is a PWM OUTPUT (0x01 and written with analogWrite)"
	digitalOutputPeripheral = 5, //"pin is a concrete class managed peripheral digital OUTPUT e.g. DigistatMk2_433 etc
	digitalInputPeripheral = 6 //"pin is a concrete class managed peripheral digital INPUT e.g. DHT
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

enum peripheralType {
	unspecified,
	digistatMk2,
	dht22,
	maxPeripheralType = dht22
};

struct ioLoggingData {

	bool logToThingSpeak;
	unsigned long thingSpeakChannel;
	char thingSpeakApiKey[MAX_NAME_STR];
	bool httpLoggingEnabled;

};

struct ioData {

	char name[MAX_NAME_STR];
	digitalMode pinMode;

	int defaultValue;
	int lastValue;

};

struct peripheralData {

	peripheralType type;
	uint8_t pinIdx; //describes the (optional) pin used for the peripheral

	float lastAnalogValue1;
	float lastAnalogValue2;

	ioData base;

};

struct gpioStorage {

	//11 digital pins are available on an ESP12 however only 'safe' GPIO are surfaced
	//by this project, those defined by Kolban as low risk (page 125 of the book)
	ioData digitals[MAX_DEVICES];

	//1 analog pin is available on an ESP12
	char analogName[MAX_NAME_STR];
	analogMode analogPinMode;
	int analogRawValue; //raw analog value between 0 and 1024 on the ESP8266
	double analogVoltage; //between 0 and 1v on the ESP8266
	double analogOffset; //offset for calculated value
	double analogMultiplier; //multiplier for calculated value
	double analogCalcVal; //resulting calculated value
	unitOfMeasure analogUnit;

	//ioLoggingData analogLogging;

};

struct peripheralStorage {

	//the configurable peripherals collection of peripheralType
	peripheralData peripherals[MAX_DEVICES];

};

#endif /* GPIODATA_H_ */
