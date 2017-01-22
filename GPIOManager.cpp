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
 * defines the digital pin map for the appConfigData.digitalPinMode
 * 11 digital pins are available on an ESP12 however only 'safe' GPIO are surfaced
 * by this project - those pins defined by Kolban as low risk (page 125 of the book)
 * for pin details see:
 * https://github.com/esp8266/Arduino/blob/master/doc/reference.md#digital-io
 * http://www.esp8266.com/wiki/doku.php?id=esp8266-module-family#esp-12-e_q
 */
const uint8_t GPIOManager::digitalPinMap[MAX_DEVICES] =
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

	lastProcess = 0;
	lastDHTRead = 0;
	initialized = false;

	digistat = NULL;
	dht = NULL;

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

	if (digistat != NULL) {
		delete digistat;
		digistat = NULL;
	}

	if (dht != NULL) {
		delete dht;
		dht = NULL;
	}

}

/*
 * Initializes digital IO pins according to currently stored config
 */
void ICACHE_FLASH_ATTR GPIOManager::initialize() {

	if (appConfigData.initialized) {

		APP_SERIAL_DEBUG("GPIOManager::initialize\n");

		if (digistat != NULL) {
			delete digistat;
			digistat = NULL;
		}

		if (dht != NULL) {
			delete dht;
			dht = NULL;
		}

		//initialise devices
		for (byte deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {

			uint8_t mappedPin = digitalPinMap[deviceIdx];

			ioData digital = appConfigData.gpio.digitals[deviceIdx];
			peripheralData *peripheral = &appConfigData.device.peripherals[deviceIdx];

			switch (digital.pinMode) {

				case digitalMode::digitalInput:

					APP_SERIAL_DEBUG("Pin %d (idx:%d, '%s') is INPUT\n", mappedPin, deviceIdx, digital.name);
					pinMode(mappedPin, INPUT);

					break;
				case digitalMode::digitalInputPullup:

					APP_SERIAL_DEBUG("Pin %d (idx:%d, '%s') is INPUT_PULLUP\n", mappedPin, deviceIdx, digital.name);
					pinMode(mappedPin, INPUT_PULLUP);

					break;
				case digitalMode::digitalOutput:
				case digitalMode::digitalAnalogOutputPwm:

					APP_SERIAL_DEBUG("Pin %d (idx:%d, '%s') is OUTPUT\n", mappedPin, deviceIdx, digital.name);
					pinMode(mappedPin, OUTPUT);

					//initialize the output to the configured default
					if (digital.pinMode == digitalOutput) {
						//initial digitalWrite on the pin configured as a digitalOutput
						gpioDigitalWrite(deviceIdx, (uint8_t) digital.defaultValue);

					} else {
						//initial analogWrite on the pin configured as digitalAnalogPwm
						gpioAnalogWrite(deviceIdx, digital.defaultValue);
					}
					break;

				default:
					//digitalNotInUse
					break;
			}

			switch (peripheral->type) {
				case peripheralType::digistatMk2:
					if (digistat == NULL) {
						digistat = new DigistatMk2_433(peripheral);
						gpioDigitalWrite(deviceIdx, (uint8_t)peripheral->base.defaultValue);
					}
					break;
				case peripheralType::dht22:
					if (dht == NULL) {
						dht = new DHT(mappedPin, DHT22);
						dht->begin();
					}
					break;
				default:
					//no others yet supported
					break;
			}

		}
		APP_SERIAL_DEBUG("GPIOManager::initialize done\n");
		initialized = true;
	}
}

/*
 * attempts to configure a peripheral for the given parameters in a free appConfigData.device.peripherals slot
 */
bool ICACHE_FLASH_ATTR GPIOManager::addPeripheral(peripheralType pType, String &peripheralName, uint8_t pinIdx, int defaultValue) {

	bool peripheralAdded = false;

	if (pType != peripheralType::unspecified) {

		peripheralData *peripheral = NULL;

		for(byte deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {

			peripheral = &appConfigData.device.peripherals[deviceIdx];

			if ((peripheral->type == peripheralType::unspecified)) {

				peripheral->type = pType;
				strncpy(peripheral->base.name , peripheralName.c_str(), STRMAX);
				peripheral->pinIdx = pinIdx;
				peripheral->base.defaultValue = defaultValue;
				peripheral->base.lastValue = UNDEFINED_GPIO;
				peripheral->lastAnalogValue1 = UNDEFINED_GPIO;
				peripheral->lastAnalogValue2 = UNDEFINED_GPIO;

				switch (pType) {
					case peripheralType::digistatMk2:
						peripheral->base.pinMode = digitalMode::digitalOutputPeripheral;
						appConfigData.gpio.digitals[deviceIdx].pinMode = digitalMode::digitalOutputPeripheral;
						strncpy(appConfigData.gpio.digitals[deviceIdx].name , peripheralName.c_str(), STRMAX);

						if (NetworkSvcMngr.mqttEnabled) {
							NetworkSvcMngr.mqttManager->subscribe(digitalMode::digitalOutputPeripheral, deviceIdx);
						}

						break;
					case peripheralType::dht22:
						peripheral->base.pinMode = digitalMode::digitalInputPeripheral;
						appConfigData.gpio.digitals[deviceIdx].pinMode = digitalMode::digitalInputPeripheral;
						strncpy(appConfigData.gpio.digitals[deviceIdx].name , peripheralName.c_str(), STRMAX);
						break;
					default:
						break;
				}

				peripheralAdded = true;
				initialize();

				break;
			}
		}
	}

	return peripheralAdded;

}

/*
 * removes the peripheral in the appConfigData.device.peripherals[deviceIdx] slot
 */
bool ICACHE_FLASH_ATTR GPIOManager::removePeripheral(uint8_t deviceIdx) {

	bool peripheralRemoved = false;

	if (deviceIdx < MAX_DEVICES) {

		peripheralData *peripheral = &appConfigData.device.peripherals[deviceIdx];

		strncpy(peripheral->base.name , EMPTY_STR, STRMAX);

		peripheral->base.pinMode = digitalMode::digitalNotInUse;
		appConfigData.gpio.digitals[deviceIdx].pinMode = digitalMode::digitalNotInUse;

		String name = DEVICE_IO_PREFIX + String(deviceIdx);
		strncpy(appConfigData.gpio.digitals[deviceIdx].name , name.c_str(), STRMAX);

		peripheral->base.defaultValue = 0;
		peripheral->base.lastValue = UNDEFINED_GPIO;

		peripheral->type = peripheralType::unspecified;
		peripheral->pinIdx = 0;
		peripheral->lastAnalogValue1 = UNDEFINED_GPIO;
		peripheral->lastAnalogValue2 = UNDEFINED_GPIO;

		initialize();

		peripheralRemoved = true;

	}

	return peripheralRemoved;

}

/*
 * removes all peripherals from all appConfigData.device.peripherals[deviceIdx] slots where the
 * peripheral->pinIdx matches the given pinIdx
 */
bool ICACHE_FLASH_ATTR GPIOManager::removePeripheralsByPinIdx(uint8_t pinIdx) {

	bool peripheralRemoved = false;

	peripheralData *peripheral = NULL;

	for(byte deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {

		peripheral = &appConfigData.device.peripherals[deviceIdx];

		if (peripheral->pinIdx == pinIdx) {

			peripheralRemoved |= removePeripheral(deviceIdx);

			break;
		}
	}

	return peripheralRemoved;

}

/*
 * returns the first peripheral where the peripheral->pinIdx matches the given pinIdx
 */
peripheralData ICACHE_FLASH_ATTR *GPIOManager::getPeripheralByPinIdx(uint8_t pinIdx) {

	peripheralData *peripheral = NULL;

	for(byte deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {

		peripheral = &appConfigData.device.peripherals[deviceIdx];

		if (peripheral->type != peripheralType::unspecified && peripheral->pinIdx == pinIdx) {

			return peripheral;
		}
	}

	return NULL;

}

/*
 * returns a GPIO digital pin value either via a true digitalRead function call or via internal cached pinState values
 * the pin must be configured as input, output or pwm analog. If not the UNDEFINED_GPIO value is returned
 */
int ICACHE_FLASH_ATTR GPIOManager::gpioDigitalRead(uint8_t pinIdx) {

	int state = UNDEFINED_GPIO;

	if (pinIdx >= 0 && pinIdx < MAX_DEVICES) {

		uint8_t mappedPin = digitalPinMap[pinIdx];

		switch (appConfigData.gpio.digitals[pinIdx].pinMode) {
		case digitalInput:
		case digitalInputPullup:
			//return the actual read state
			state = digitalRead(mappedPin);
			if (appConfigData.gpio.digitals[pinIdx].lastValue != state) {
				appConfigData.gpio.digitals[pinIdx].lastValue = state;

				NetworkSvcMngr.broadcastDeviceStateChange(ioType::digital, pinIdx);
			}
			break;
		case digitalOutput:
		case digitalAnalogOutputPwm:
			//return the internally cached last written state
			state = appConfigData.gpio.digitals[pinIdx].lastValue;
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

		if ((appConfigData.gpio.analogRawValue != rawVal) ||
			(appConfigData.powerMgmt.enabled && appConfigData.powerMgmt.onLength == powerOnLength::zeroLength)) {

			appConfigData.gpio.analogRawValue = rawVal;
			appConfigData.gpio.analogVoltage = appConfigData.gpio.analogRawValue / 1024.0;
			appConfigData.gpio.analogCalcVal = (appConfigData.gpio.analogVoltage + appConfigData.gpio.analogOffset) * appConfigData.gpio.analogMultiplier;

			NetworkSvcMngr.broadcastDeviceStateChange(ioType::analog, 0);
		}
	}

	return rawVal;
}

/*
 * Writes a digital GPIO value to a pin and stores the value.
 * The pin must be configured as digitalOutput and the given value must be either LOW or HIGH
 */
bool ICACHE_FLASH_ATTR GPIOManager::gpioDigitalWrite(
		uint8_t pinIdx,
		uint8_t value,
		bool suppressMqtt) {

	if (pinIdx >= 0 && pinIdx < MAX_DEVICES) {

		if (value == LOW || value == HIGH) {

			if (appConfigData.gpio.digitals[pinIdx].pinMode == digitalMode::digitalOutput) {

				uint8_t mappedPin = digitalPinMap[pinIdx];
				APP_SERIAL_DEBUG("Writing digital output value %d for pin %d (idx:%d, '%s')\n", value, mappedPin, pinIdx, appConfigData.gpio.digitals[pinIdx].name);

				digitalWrite(mappedPin, value);
				appConfigData.gpio.digitals[pinIdx].lastValue = value;

				NetworkSvcMngr.broadcastDeviceStateChange(
					ioType::digital,
					pinIdx,
					peripheralType::unspecified,
					!suppressMqtt);

			} else if (appConfigData.gpio.digitals[pinIdx].pinMode == digitalMode::digitalOutputPeripheral) {

				if (digistat != NULL) {
					digistat->Switch(value);
					appConfigData.gpio.digitals[pinIdx].lastValue = value;

					NetworkSvcMngr.broadcastDeviceStateChange(
						ioType::digital,
						pinIdx,
						peripheralType::digistatMk2,
						!suppressMqtt);

				}
			}

			return true;
		}
	}
	return false;
}

/*
 * Writes an analog GPIO value to a pin and stores the value.
 * The pin must be configured as digitalAnalogOutputPwm and the given value must be between MIN_ANALOG and MAX_ANALOG
 */
bool ICACHE_FLASH_ATTR GPIOManager::gpioAnalogWrite(
		uint8_t pinIdx,
		int value,
		bool suppressMqtt) {

	if (pinIdx >= 0 && pinIdx < MAX_DEVICES) {

		uint8_t mappedPin = digitalPinMap[pinIdx];

		if (appConfigData.gpio.digitals[pinIdx].pinMode == digitalAnalogOutputPwm) {

			if (value >= MIN_ANALOG && value <= MAX_ANALOG) {

				APP_SERIAL_DEBUG("Writing analog PWM output value %d for pin %d (idx:%d, '%s')\n", value, mappedPin, pinIdx, appConfigData.gpio.digitals[pinIdx].name);

				analogWrite(mappedPin, value);
				appConfigData.gpio.digitals[pinIdx].lastValue = value;

				NetworkSvcMngr.broadcastDeviceStateChange(
					ioType::digital,
					pinIdx,
					peripheralType::unspecified,
					!suppressMqtt);

				return true;
			}
		}
	}
	return false;
}

/*
 * returns the analog A0 pin value via an analogRead function call
 * analogPinMode must == analogEnabled otherwise the UNDEFINED_GPIO value is returned
 */
void ICACHE_FLASH_ATTR GPIOManager::dhtRead(uint8_t deviceIdx) {

	peripheralData *peripheral = &appConfigData.device.peripherals[deviceIdx];

	if (dht != NULL && peripheral->type == peripheralType::dht22) {

		APP_SERIAL_DEBUG("dhtRead\n");

		float celcius = dht->readTemperature();
		float humidity = dht->readHumidity();

		if (isnan(humidity) || isnan(celcius)) {
			APP_SERIAL_DEBUG("dhtRead failure\n");
			return;
		}

		if (peripheral->lastAnalogValue1 != celcius || peripheral->lastAnalogValue2 != humidity) {

			peripheral->lastAnalogValue1 = celcius;
			peripheral->lastAnalogValue2 = humidity;

			NetworkSvcMngr.broadcastDeviceStateChange(ioType::digital, deviceIdx, peripheralType::dht22);

		}
	}
}

/*
 * reads/writes configured devices
 * this is a loop function
 */
void ICACHE_FLASH_ATTR GPIOManager::processGPIO() {

	unsigned long now = millis();

	if ((initialized) && ((now - lastProcess > 1000) || (lastProcess == 0))) {

		for (byte deviceIdx = 0; deviceIdx < MAX_DEVICES; deviceIdx++) {
			if ((appConfigData.gpio.digitals[deviceIdx].pinMode == digitalInput) || (appConfigData.gpio.digitals[deviceIdx].pinMode == digitalInputPullup)) {

				gpioDigitalRead(deviceIdx);

			} else {

				peripheralData *peripheral = &appConfigData.device.peripherals[deviceIdx];

				switch (peripheral->type) {
					case peripheralType::digistatMk2:
						if (digistat != NULL) {
							digistat->Repeat();
						}
						break;
					case peripheralType::dht22:
						if ((now - lastDHTRead > 30000) || (lastDHTRead == 0)) {
							dhtRead(deviceIdx);
							lastDHTRead = now;
						}
						break;
					default:
						break;
				}
			}
		}

		if (appConfigData.gpio.analogPinMode == analogEnabled)
			gpioAnalogRead();

		lastProcess = now;

	}
}

String ICACHE_FLASH_ATTR GPIOManager::getUnitOfMeasureStr(unitOfMeasure unit) {

	switch (unit) {
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

