/*
 * gpioManager.cpp
 *
 *  Created on: 17 Mar 2016
 *      Author: joe
 */

#include "GPIOManager.h"

#include "flashAppData.h"
#include "utils.h"
#include "NetworkServices.h"

/*
 * The global gpioManager instance
 */
GPIOManager GPIOMngr;

/*
void gpioInterrupt1(){
	debugSerialPrint("gpioInterrupt1");
	debugAPP_SERIAL_DEBUG(GPIOMngr.digitalPinMap[0]);
}
void gpioInterrupt2(){
	debugSerialPrint("gpioInterrupt2");
	debugAPP_SERIAL_DEBUG(GPIOMngr.digitalPinMap[1]);
}
void gpioInterrupt3(){
	debugSerialPrint("gpioInterrupt3");
	debugAPP_SERIAL_DEBUG(GPIOMngr.digitalPinMap[2]);
}
void gpioInterrupt4(){
	debugSerialPrint("gpioInterrupt4");
	debugAPP_SERIAL_DEBUG(GPIOMngr.digitalPinMap[3]);
}
void gpioInterrupt5(){
	debugSerialPrint("gpioInterrupt5");
	debugAPP_SERIAL_DEBUG(GPIOMngr.digitalPinMap[4]);
}

void (*interruptFunc[5]) (void);
*/

/*
 * defines the digital pin map for the appConfigData.digitalPinMode
 * 11 digital pins are available on an ESP12 however only 'safe' GPIO are surfaced
 * by this project - those pins defined by Kolban as low risk (page 125 of the book)
 * for pin details see:
 * https://github.com/esp8266/Arduino/blob/master/doc/reference.md#digital-io
 * http://www.esp8266.com/wiki/doku.php?id=esp8266-module-family#esp-12-e_q
 */
const uint8_t GPIOManager::digitalPinMap[MAX_GPIO] =
{
	12,
	13,
	14,
	4, //acts as GPIO or I2C SDA
	5, //acts as GPIO or I2C SCL
};

/*
 * constructor
 */
ICACHE_FLASH_ATTR GPIOManager::GPIOManager() {

	lastInputRead = 0;
	initialized = false;

	/*
	interruptFunc[0] = gpioInterrupt1;
	interruptFunc[1] = gpioInterrupt2;
	interruptFunc[2] = gpioInterrupt3;
	interruptFunc[3] = gpioInterrupt4;
	interruptFunc[4] = gpioInterrupt5;
	 */

	analogReadIndex = 0;
	analogRunningTotal = 0;
	stabilizationArrayFull = false;

	for (int i = 0; i < MAX_ANALOG_READINGS; i++) {
		analogStabilizationArray[i] = 0;
	}
}

/*
 * destructor
 */
ICACHE_FLASH_ATTR GPIOManager::~GPIOManager() {

}

/*
 * Initializes digital IO pins according to currently stored config
 */
void ICACHE_FLASH_ATTR GPIOManager::initialize() {

	if(appConfigData.initialized) {

		APP_SERIAL_DEBUG("GPIOManager::initialize\n");

		//initialise digital IO pins
		for(int pinIdx = 0; pinIdx < MAX_GPIO; pinIdx++) {

			uint8_t mappedPin = digitalPinMap[pinIdx];
			digitalData digitalIO = appConfigData.gpio.digitalIO[pinIdx];

			switch (digitalIO.digitalPinMode)
			{
			case digitalInput:

				APP_SERIAL_DEBUG("Pin %d (idx:%d, '%s') is INPUT\n", mappedPin, pinIdx, digitalIO.digitalName);
				pinMode(mappedPin, INPUT);

				//TODO: attachInterrupt for CHANGE
				//attachInterrupt(mappedPin, interruptFunc[i], CHANGE);
				break;
			case digitalInputPullup:

				APP_SERIAL_DEBUG("Pin %d (idx:%d, '%s') is INPUT_PULLUP\n", mappedPin, pinIdx, digitalIO.digitalName);
				pinMode(mappedPin, INPUT_PULLUP);

				//TODO: attachInterrupt for CHANGE
				//attachInterrupt(mappedPin, interruptFunc[i], CHANGE);
				break;
			case digitalOutput:
			case digitalAnalogOutputPwm:

				APP_SERIAL_DEBUG("Pin %d (idx:%d, '%s') is OUTPUT\n", mappedPin, pinIdx, digitalIO.digitalName);
				pinMode(mappedPin, OUTPUT);

				//initialize the output to the configured default
				if (digitalIO.digitalPinMode == digitalOutput) {

					//initial digitalWrite on the pin configured as a digitalOutput
					gpioDigitalWrite(pinIdx, (uint8_t)digitalIO.defaultValue);

				} else {

					//initial analogWrite on the pin configured as digitalAnalogPwm
					gpioAnalogWrite(pinIdx, digitalIO.defaultValue);

				}
				//TODO: attachInterrupt for CHANGE
				//attachInterrupt(mappedPin, interruptFunc[i], CHANGE);
				break;
			default:
				//digitalNotInUse
				break;
			}
		}
		initialized = true;
	}
}

/*
 * returns a GPIO digital pin value either via a true digitalRead function call or via internal cached pinState values
 * the pin must be configured as input, output or pwm analog. If not the UNDEFINED_GPIO value is returned
 */
int ICACHE_FLASH_ATTR GPIOManager::gpioDigitalRead(uint8_t pinIdx) {

	int state = UNDEFINED_GPIO;

	if(pinIdx >= 0 && pinIdx < MAX_GPIO) {

		uint8_t mappedPin = digitalPinMap[pinIdx];

		switch (appConfigData.gpio.digitalIO[pinIdx].digitalPinMode)
		{
		case digitalInput:
		case digitalInputPullup:
			//return the actual read state
			state = digitalRead(mappedPin);
			if(appConfigData.gpio.digitalIO[pinIdx].lastValue != state) {
				appConfigData.gpio.digitalIO[pinIdx].lastValue = state;

				NetworkServiceManager.broadcastGPIOChange(
					digital,
					pinIdx);
			}
			break;
		case digitalOutput:
		case digitalAnalogOutputPwm:
			//return the internally cached last written state
			state = appConfigData.gpio.digitalIO[pinIdx].lastValue;
			break;
		default:
			//digitalNotInUse
			break;
		}

	}

	return state;
}

/*
 * returns the analog A0 pin value via an analogRead function call
 * analogPinMode must == analogEnabled otherwise the UNDEFINED_GPIO value is returned
 */
int ICACHE_FLASH_ATTR GPIOManager::gpioAnalogRead() {

	int rawVal = UNDEFINED_GPIO;

	if (appConfigData.gpio.analogPinMode == analogEnabled) {

		APP_SERIAL_DEBUG("Analog read\n");

		//analog raw value averaging / stabilization is currently ON by default
		if (analogReadIndex >= MAX_ANALOG_READINGS) {
			// wrap around
			analogReadIndex = 0;
			stabilizationArrayFull = true;
		}

		analogRunningTotal = analogRunningTotal - analogStabilizationArray[analogReadIndex];

		analogStabilizationArray[analogReadIndex] = analogRead(A0);

		analogRunningTotal = analogRunningTotal + analogStabilizationArray[analogReadIndex];

		analogReadIndex += 1;

		rawVal = !stabilizationArrayFull
				? analogRunningTotal / analogReadIndex
				: analogRunningTotal / MAX_ANALOG_READINGS;

		if ((appConfigData.gpio.analogRawValue != rawVal) || (appConfigData.powerMgmt.enabled && appConfigData.powerMgmt.onLength == powerOnLength::zeroLength)) {

			appConfigData.gpio.analogRawValue = rawVal;
			appConfigData.gpio.analogVoltage = appConfigData.gpio.analogRawValue / 1024.0;
			appConfigData.gpio.analogCalcVal = (appConfigData.gpio.analogVoltage + appConfigData.gpio.analogOffset) * appConfigData.gpio.analogMultiplier;

			NetworkServiceManager.broadcastGPIOChange(
				analog,
				0);
		}
	}

	return rawVal;
}

/*
 * Writes a digital GPIO value to a pin and stores the value.
 * The pin must be configured as digitalOutput and the given value must be either LOW or HIGH
 */
bool ICACHE_FLASH_ATTR GPIOManager::gpioDigitalWrite(uint8_t pinIdx, uint8_t value, bool suppressMqtt) {

	if(pinIdx >= 0 && pinIdx < MAX_GPIO) {

		uint8_t mappedPin = digitalPinMap[pinIdx];

		if (appConfigData.gpio.digitalIO[pinIdx].digitalPinMode == digitalOutput) {

			if(value == LOW || value == HIGH) {

				APP_SERIAL_DEBUG("Writing digital output value %d for pin %d (idx:%d, '%s')\n", value, mappedPin, pinIdx, appConfigData.gpio.digitalIO[pinIdx].digitalName);

				digitalWrite(mappedPin, value);
				appConfigData.gpio.digitalIO[pinIdx].lastValue = value;

				NetworkServiceManager.broadcastGPIOChange(
					digital,
					pinIdx,
					!suppressMqtt);

				return true;
			}
		}
	}
	return false;
}

/*
 * Writes an analog GPIO value to a pin and stores the value.
 * The pin must be configured as digitalAnalogOutputPwm and the given value must be between MIN_ANALOG and MAX_ANALOG
 */
bool ICACHE_FLASH_ATTR GPIOManager::gpioAnalogWrite(uint8_t pinIdx, int value, bool suppressMqtt) {

	if(pinIdx >= 0 && pinIdx < MAX_GPIO) {

		uint8_t mappedPin = digitalPinMap[pinIdx];

		if (appConfigData.gpio.digitalIO[pinIdx].digitalPinMode == digitalAnalogOutputPwm) {

			if(value >= MIN_ANALOG && value <= MAX_ANALOG) {

				APP_SERIAL_DEBUG("Writing analog PWM output value %d for pin %d (idx:%d, '%s')\n", value, mappedPin, pinIdx, appConfigData.gpio.digitalIO[pinIdx].digitalName);

				analogWrite(mappedPin, value);
				appConfigData.gpio.digitalIO[pinIdx].lastValue = value;

				NetworkServiceManager.broadcastGPIOChange(
					digital,
					pinIdx,
					!suppressMqtt);

				return true;
			}
		}
	}
	return false;
}


/*
 * reads configured GPIO inputs and broadcasts GPIO state to websocket connected clients
 * this is a loop function
 */
void ICACHE_FLASH_ATTR GPIOManager::readInputs() {

	if ((initialized) && ((millis() - lastInputRead > 1000) || (lastInputRead == 0))) {

		for(int pinIdx = 0; pinIdx < MAX_GPIO; pinIdx++) {
			if ((appConfigData.gpio.digitalIO[pinIdx].digitalPinMode == digitalInput) || (appConfigData.gpio.digitalIO[pinIdx].digitalPinMode == digitalInputPullup))
				gpioDigitalRead(pinIdx);
		}

		if(appConfigData.gpio.analogPinMode == analogEnabled)
			gpioAnalogRead();

		lastInputRead = millis();

	}
}

String ICACHE_FLASH_ATTR GPIOManager::getUnitOfMeasureStr(unitOfMeasure unit) {

	switch(unit)
	{
		case none:
			return EMPTY_STR;
		case celcius:
			return getAppStr(appStrType::celciusStr);
		case fahrenheit:
			return getAppStr(appStrType::fahrenheitStr);
		case millimeter:
			return getAppStr(appStrType::mm);
		case centimeter:
			return getAppStr(appStrType::cm);
		case metre:
			return getAppStr(appStrType::m);
		case kilometer:
			return getAppStr(appStrType::km);
		case inch:
			return getAppStr(appStrType::in);
		case foot:
			return getAppStr(appStrType::ft);
		case mile:
			return getAppStr(appStrType::mi);
		case millilitre:
			return getAppStr(appStrType::ml);
		case centilitre:
			return getAppStr(appStrType::cl);
		case litre:
			return getAppStr(appStrType::l);
		case gallon:
			return getAppStr(appStrType::gal);
		case milligram:
			return getAppStr(appStrType::mg);
		case gram:
			return getAppStr(appStrType::g);
		case kilogram:
			return getAppStr(appStrType::kg);
		case ounce:
			return getAppStr(appStrType::oz);
		case pound:
			return getAppStr(appStrType::lb);
		case ton:
			return getAppStr(appStrType::t);
		case btu:
			return getAppStr(appStrType::BTU);
		case millivolt:
			return getAppStr(appStrType::mV);
		case volt:
			return getAppStr(appStrType::v);
		case milliampere:
			return getAppStr(appStrType::mA);
		case ampere:
			return getAppStr(appStrType::A);
		case ohm:
			return getAppStr(appStrType::ohms);
		case watt:
			return getAppStr(appStrType::W);
		default:
			return EMPTY_STR;
	}

}


