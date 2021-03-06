#ifndef DallasTemperature_h
#define DallasTemperature_h

#define DALLASTEMPLIBVERSION "3.7.2"

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// set to true to include code for new and delete operators
#ifndef REQUIRESNEW
#define REQUIRESNEW false
#endif

// set to true to include code implementing alarm search functions
#ifndef REQUIRESALARMS
#define REQUIRESALARMS true
#endif

#include "stm8s.h"
#include "one_wire.h"

// Model IDs
#define DS18S20MODEL 0x10
#define DS18B20MODEL 0x28
#define DS1822MODEL  0x22

// OneWire commands
#define STARTCONVO      0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define COPYSCRATCH     0x48  // Copy EEPROM
#define READSCRATCH     0xBE  // Read EEPROM
#define WRITESCRATCH    0x4E  // Write to EEPROM
#define RECALLSCRATCH   0xB8  // Reload from last known
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition

// Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8

// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

// Error Codes
#define DEVICE_DISCONNECTED -127

//constrain functions
#define constrain(x, a, b) ((x < a) ? a : ((x > b) ? b : x))

// max function
#define max(a, b) ((a < b) ? a : b)

typedef uint8_t DeviceAddress[8];

struct DallasTemperature
{
	void (*Init)(struct OneWire * one_wire);
	
	// initalise bus
	void (*begin)(void);
	
	// returns the number of devices found on the bus
	uint8_t (*getDeviceCount)(void);
	
	// Is a conversion complete on the wire?
	bool (*isConversionComplete)(void);
	
	// returns true if address is valid
	bool (*validAddress)(uint8_t*);
	
	// finds an address at a given index on the bus 
	bool (*getAddress)(uint8_t*, const uint8_t);
	
	// attempt to determine if the device at the given address is connected to the bus
	bool (*isConnected1)(uint8_t*);
	
	// attempt to determine if the device at the given address is connected to the bus
	// also allows for updating the read scratchpad
	bool (*isConnected2)(uint8_t*, uint8_t*);
	
	// read device's scratchpad
	void (*readScratchPad)(uint8_t*, uint8_t*);
	
	// write device's scratchpad
	void (*writeScratchPad)(uint8_t*, const uint8_t*);
	
	// read device's power requirements
	bool (*readPowerSupply)(uint8_t*);
	
	// get global resolution
	uint8_t (*getResolution1)(void);
	
	// set global resolution to 9, 10, 11, or 12 bits
	void (*setResolution1)(uint8_t);
	
	// returns the device resolution, 9-12
	uint8_t (*getResolution2)(uint8_t*);
	
	// set resolution of a device to 9, 10, 11, or 12 bits
	bool (*setResolution2)(uint8_t*, uint8_t);
	
	// sets/gets the waitForConversion flag
	void (*setWaitForConversion)(bool);
	bool (*getWaitForConversion)(void);
	
	// sets/gets the checkForConversion flag
	void (*setCheckForConversion)(bool);
	bool (*getCheckForConversion)(void);
	
	// sends command for all devices on the bus to perform a temperature conversion 
	void (*requestTemperatures)(void);
	
	// sends command for one device to perform a temperature conversion by address
	bool (*requestTemperaturesByAddress)(uint8_t*);
	
	// sends command for one device to perform a temperature conversion by index
	bool (*requestTemperaturesByIndex)(uint8_t);
	
	// returns temperature in degrees C
	float (*getTempC)(uint8_t*);
	
	// returns temperature in degrees F
	float (*getTempF)(uint8_t*);
	
	// Get temperature for device index (slow)
	float (*getTempCByIndex)(uint8_t);
	
	// Get temperature for device index (slow)
	float (*getTempFByIndex)(uint8_t);
	
	// returns true if the bus requires parasite power
	bool (*isParasitePowerMode)(void);
	
	bool (*isConversionAvailable)(uint8_t*);
	
#if REQUIRESALARMS
	
	typedef void AlarmHandler(uint8_t*);
	
	// sets the high alarm temperature for a device
	// accepts a char.  valid range is -55C - 125C
	void (*setHighAlarmTemp)(uint8_t*, const int8_t);
	
	// sets the low alarm temperature for a device
	// accepts a char.  valid range is -55C - 125C
	void (*setLowAlarmTemp)(uint8_t*, const int8_t);
	
	// returns a signed char with the current high alarm temperature for a device
	// in the range -55C - 125C
	int8_t (*getHighAlarmTemp)(uint8_t*);
	
	// returns a signed char with the current low alarm temperature for a device
	// in the range -55C - 125C
	int8_t (*getLowAlarmTemp)(uint8_t*);
	
	// resets internal variables used for the alarm search
	void (*resetAlarmSearch)(void);
	
	// search the wire for devices with active alarms
	bool (*alarmSearch)(uint8_t*);
	
	// returns true if ia specific device has an alarm
	bool (*hasAlarm1)(uint8_t*);
	
	// returns true if any device is reporting an alarm on the bus
	bool (*hasAlarm2)(void);
	
	// runs the alarm handler for all devices returned by alarmSearch()
	void (*processAlarms)(void);
	
	// sets the alarm handler
	void (*setAlarmHandler)(AlarmHandler *);
	
	// The default alarm handler
	void (*defaultAlarmHandler)(uint8_t*);
	
#endif
	
	// convert from celcius to farenheit
	float (*toFahrenheit)(const float);
	
	// convert from farenheit to celsius
	float (*toCelsius)(const float);
	
#if REQUIRESNEW
	
	// initalize memory area
	void* operator new (unsigned int);
	
	// delete memory reference
	void operator delete(void*);
	
#endif
};

extern struct DallasTemperature DS18B20;
#endif
