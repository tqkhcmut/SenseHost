// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// Version 3.7.2 modified on Dec 6, 2011 to support Arduino 1.0
// See Includes...
// Modified by Jordan Hochenbaum

#include "DallasTemperature.h"
#include "delay.h"

typedef uint8_t ScratchPad[9];

// parasite power on or off
bool parasite;

// used to determine the delay amount needed to allow for the
// temperature conversion to take place
uint8_t bitResolution;

// used to requestTemperature with or without delay
bool waitForConversion;

// used to requestTemperature to dynamically check if a conversion is complete
bool checkForConversion;

// count of devices on the bus
uint8_t devices;

// Take a pointer to one wire instance
struct OneWire * _wire;


// reads scratchpad and returns the temperature in degrees C
float calculateTemperature(uint8_t*, uint8_t*);

void	blockTillConversionComplete(uint8_t*,uint8_t*);

#if REQUIRESALARMS

// required for alarmSearch 
uint8_t alarmSearchAddress[8];
int8_t alarmSearchJunction;
uint8_t alarmSearchExhausted;

// the alarm handler function pointer
AlarmHandler *_AlarmHandler;

#endif


void DallasTemperature_Init(struct OneWire* _oneWire)
#if REQUIRESALARMS
: _AlarmHandler(&defaultAlarmHandler)
#endif
{
	_wire = _oneWire;
	devices = 0;
	parasite = FALSE;
	bitResolution = 9;
	waitForConversion = TRUE;
	checkForConversion = TRUE;
}

// returns the number of devices found on the bus
uint8_t DallasTemperature_getDeviceCount(void)
{
	return devices;
}

// returns TRUE if address is valid
bool DallasTemperature_validAddress(uint8_t* deviceAddress)
{
	return (bool)(_wire->crc8(deviceAddress, 7) == deviceAddress[7]);
}

// finds an address at a given index on the bus
// returns TRUE if the device was found
bool DallasTemperature_getAddress(uint8_t* deviceAddress, uint8_t index)
{
	uint8_t depth = 0;
	
	_wire->reset_search();
	
	while (depth <= index && _wire->search(deviceAddress))
	{
		if (depth == index && DallasTemperature_validAddress(deviceAddress)) return TRUE;
		depth++;
	}
	
	return FALSE;
}

// read device's scratch pad
void DallasTemperature_readScratchPad(uint8_t* deviceAddress, uint8_t* scratchPad)
{
	// send the command
	_wire->reset();
	_wire->select(deviceAddress);
	_wire->write(READSCRATCH, 0);
	
	// TODO => collect all comments &  use simple loop
	// byte 0: temperature LSB  
	// byte 1: temperature MSB
	// byte 2: high alarm temp
	// byte 3: low alarm temp
	// byte 4: DS18S20: store for crc
	//         DS18B20 & DS1822: configuration register
	// byte 5: internal use & crc
	// byte 6: DS18S20: COUNT_REMAIN
	//         DS18B20 & DS1822: store for crc
	// byte 7: DS18S20: COUNT_PER_C
	//         DS18B20 & DS1822: store for crc
	// byte 8: SCRATCHPAD_CRC
	//
	// for(int i=0; i<9; i++)
	// {
	//   scratchPad[i] = _wire->read();
	// }
	
	
	// read the response
	
	// byte 0: temperature LSB
	scratchPad[TEMP_LSB] = _wire->read();
	
	// byte 1: temperature MSB
	scratchPad[TEMP_MSB] = _wire->read();
	
	// byte 2: high alarm temp
	scratchPad[HIGH_ALARM_TEMP] = _wire->read();
	
	// byte 3: low alarm temp
	scratchPad[LOW_ALARM_TEMP] = _wire->read();
	
	// byte 4:
	// DS18S20: store for crc
	// DS18B20 & DS1822: configuration register
	scratchPad[CONFIGURATION] = _wire->read();
	
	// byte 5:
	// internal use & crc
	scratchPad[INTERNAL_BYTE] = _wire->read();
	
	// byte 6:
	// DS18S20: COUNT_REMAIN
	// DS18B20 & DS1822: store for crc
	scratchPad[COUNT_REMAIN] = _wire->read();
	
	// byte 7:
	// DS18S20: COUNT_PER_C
	// DS18B20 & DS1822: store for crc
	scratchPad[COUNT_PER_C] = _wire->read();
	
	// byte 8:
	// SCTRACHPAD_CRC
	scratchPad[SCRATCHPAD_CRC] = _wire->read();
	
	_wire->reset();
}

// attempt to determine if the device at the given address is connected to the bus
// also allows for updating the read scratchpad
bool DallasTemperature_isConnected2(uint8_t* deviceAddress, uint8_t* scratchPad)
{
	DallasTemperature_readScratchPad(deviceAddress, scratchPad);
	return (bool)(_wire->crc8(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
}

// attempt to determine if the device at the given address is connected to the bus
bool DallasTemperature_isConnected1(uint8_t* deviceAddress)
{
	ScratchPad scratchPad;
	return DallasTemperature_isConnected2(deviceAddress, scratchPad);
}


// writes device's scratch pad
void DallasTemperature_writeScratchPad(uint8_t* deviceAddress, const uint8_t* scratchPad)
{
	_wire->reset();
	_wire->select(deviceAddress);
	_wire->write(WRITESCRATCH, 0);
	_wire->write(scratchPad[HIGH_ALARM_TEMP], 0); // high alarm temp
	_wire->write(scratchPad[LOW_ALARM_TEMP], 0); // low alarm temp
	// DS18S20 does not use the configuration register
	if (deviceAddress[0] != DS18S20MODEL) _wire->write(scratchPad[CONFIGURATION], 0); // configuration
	_wire->reset();
	// save the newly written values to eeprom
	_wire->write(COPYSCRATCH, parasite);
	if (parasite) _delay_ms(10); // 10ms delay
	_wire->reset();
}

// reads the device's power requirements
bool DallasTemperature_readPowerSupply(uint8_t* deviceAddress)
{
	bool ret = FALSE;
	_wire->reset();
	_wire->select(deviceAddress);
	_wire->write(READPOWERSUPPLY, 0);
	if (_wire->read_bit() == 0) ret = TRUE;
	_wire->reset();
	return ret;
}


// set resolution of a device to 9, 10, 11, or 12 bits
// if new resolution is out of range, 9 bits is used. 
bool DallasTemperature_setResolution2(uint8_t* deviceAddress, uint8_t newResolution)
{
	ScratchPad scratchPad;
	if (DallasTemperature_isConnected2(deviceAddress, scratchPad))
	{
		// DS18S20 has a fixed 9-bit resolution
		if (deviceAddress[0] != DS18S20MODEL)
		{
			switch (newResolution)
			{
			case 12:
				scratchPad[CONFIGURATION] = TEMP_12_BIT;
				break;
			case 11:
				scratchPad[CONFIGURATION] = TEMP_11_BIT;
				break;
			case 10:
				scratchPad[CONFIGURATION] = TEMP_10_BIT;
				break;
			case 9:
			default:
				scratchPad[CONFIGURATION] = TEMP_9_BIT;
				break;
			}
			DallasTemperature_writeScratchPad(deviceAddress, scratchPad);
		}
		return TRUE;  // new value set
	}
	return FALSE;
}


// set resolution of all devices to 9, 10, 11, or 12 bits
// if new resolution is out of range, it is constrained.
void DallasTemperature_setResolution1(uint8_t newResolution)
{
	bitResolution = constrain(newResolution, 9, 12);
	DeviceAddress deviceAddress;
	for (int i=0; i<devices; i++)
	{
		DallasTemperature_getAddress(deviceAddress, i);
		DallasTemperature_setResolution2(deviceAddress, bitResolution);
	}
}

// returns the global resolution
uint8_t DallasTemperature_getResolution1(void)
{
	return bitResolution;
}

// returns the current resolution of the device, 9-12
// returns 0 if device not found
uint8_t DallasTemperature_getResolution2(uint8_t* deviceAddress)
{
	if (deviceAddress[0] == DS18S20MODEL) return 9; // this model has a fixed resolution
	
	ScratchPad scratchPad;
	if (DallasTemperature_isConnected2(deviceAddress, scratchPad))
	{
		switch (scratchPad[CONFIGURATION])
		{
		case TEMP_12_BIT:
			return 12;
			
		case TEMP_11_BIT:
			return 11;
			
		case TEMP_10_BIT:
			return 10;
			
		case TEMP_9_BIT:
			return 9;
			
		}
	}
	return 0;
}


// sets the value of the waitForConversion flag
// TRUE : function requestTemperature() etc returns when conversion is ready
// FALSE: function requestTemperature() etc returns immediately (USE WITH CARE!!)
// 		  (1) programmer has to check if the needed delay has passed 
//        (2) but the application can do meaningful things in that time
void DallasTemperature_setWaitForConversion(bool flag)
{
	waitForConversion = flag;
}

// gets the value of the waitForConversion flag
bool DallasTemperature_getWaitForConversion()
{
	return waitForConversion;
}


// sets the value of the checkForConversion flag
// TRUE : function requestTemperature() etc will 'listen' to an IC to determine whether a conversion is complete
// FALSE: function requestTemperature() etc will wait a set time (worst case scenario) for a conversion to complete
void DallasTemperature_setCheckForConversion(bool flag)
{
	checkForConversion = flag;
}

// gets the value of the waitForConversion flag
bool DallasTemperature_getCheckForConversion()
{
	return checkForConversion;
}

bool DallasTemperature_isConversionAvailable(uint8_t* deviceAddress)
{
	// Check if the clock has been raised indicating the conversion is complete
	ScratchPad scratchPad;
	DallasTemperature_readScratchPad(deviceAddress, scratchPad);
	return (bool)scratchPad[0];
}	


// sends command for all devices on the bus to perform a temperature conversion
void DallasTemperature_requestTemperatures()
{
	_wire->reset();
	_wire->skip();
	_wire->write(STARTCONVO, parasite);
	
	// ASYNC mode?
	if (!waitForConversion) return; 
	blockTillConversionComplete(&bitResolution, 0);
	
	return;
}

// sends command for one device to perform a temperature by address
// returns FALSE if device is disconnected
// returns TRUE  otherwise
bool DallasTemperature_requestTemperaturesByAddress(uint8_t* deviceAddress)
{
	
	_wire->reset();
	_wire->select(deviceAddress);
	_wire->write(STARTCONVO, parasite);
	
	// check device
	ScratchPad scratchPad;
	if (!DallasTemperature_isConnected2(deviceAddress, scratchPad)) return FALSE;
	
	
	// ASYNC mode?
	if (!waitForConversion) return TRUE;   
	uint8_t bitResolution = DallasTemperature_getResolution2(deviceAddress);
	blockTillConversionComplete(&bitResolution, deviceAddress);
	
	return TRUE;
}


void blockTillConversionComplete(uint8_t* bitResolution, uint8_t* deviceAddress)
{
	if(deviceAddress != 0 && checkForConversion && !parasite)
	{
		// Continue to check if the IC has responded with a temperature
		// NB: Could cause issues with multiple devices (one device may respond faster)
		unsigned long start = millis();
		while(!DallasTemperature_isConversionAvailable(0) && ((millis() - start) < 750));	
	}
	
	// Wait a fix number of cycles till conversion is complete (based on IC datasheet)
	switch (*bitResolution)
	{
	case 9:
		_delay_ms(94);
		break;
	case 10:
		_delay_ms(188);
		break;
	case 11:
		_delay_ms(375);
		break;
	case 12:
	default:
		_delay_ms(750);
		break;
	}
	
}

// sends command for one device to perform a temp conversion by index
bool DallasTemperature_requestTemperaturesByIndex(uint8_t deviceIndex)
{
	DeviceAddress deviceAddress;
	DallasTemperature_getAddress(deviceAddress, deviceIndex);
	return DallasTemperature_requestTemperaturesByAddress(deviceAddress);
}

// returns temperature in degrees C or DEVICE_DISCONNECTED if the
// device's scratch pad cannot be read successfully.
// the numeric value of DEVICE_DISCONNECTED is defined in
// DallasTemperature.h. It is a large negative number outside the
// operating range of the device
float DallasTemperature_getTempC(uint8_t* deviceAddress)
{
	// TODO: Multiple devices (up to 64) on the same bus may take 
	//       some time to negotiate a response
	// What happens in case of collision?
	
	ScratchPad scratchPad;
	if (DallasTemperature_isConnected2(deviceAddress, scratchPad)) return calculateTemperature(deviceAddress, scratchPad);
	return DEVICE_DISCONNECTED;
}

// Fetch temperature for device index
float DallasTemperature_getTempCByIndex(uint8_t deviceIndex)
{
	DeviceAddress deviceAddress;
	DallasTemperature_getAddress(deviceAddress, deviceIndex);
	return DallasTemperature_getTempC((uint8_t*)deviceAddress);
}

// Convert float celsius to fahrenheit
float DallasTemperature_toFahrenheit(float celsius)
{
	return (celsius * 1.8) + 32;
}

// Fetch temperature for device index
float DallasTemperature_getTempFByIndex(uint8_t deviceIndex)
{
	return DallasTemperature_toFahrenheit(DallasTemperature_getTempCByIndex(deviceIndex));
}

// reads scratchpad and returns the temperature in degrees C
float calculateTemperature(uint8_t* deviceAddress, uint8_t* scratchPad)
{
	int16_t rawTemperature = (((int16_t)scratchPad[TEMP_MSB]) << 8) | scratchPad[TEMP_LSB];
	
	switch (deviceAddress[0])
	{
	case DS18B20MODEL:
	case DS1822MODEL:
		switch (scratchPad[CONFIGURATION])
		{
		case TEMP_12_BIT:
			return (float)rawTemperature * 0.0625;
			//           break;
		case TEMP_11_BIT:
			return (float)(rawTemperature >> 1) * 0.125;
			//           break;
		case TEMP_10_BIT:
			return (float)(rawTemperature >> 2) * 0.25;
			//           break;
		case TEMP_9_BIT:
			return (float)(rawTemperature >> 3) * 0.5;
			//           break;
		}
		break;
	case DS18S20MODEL:
		/*
		
		Resolutions greater than 9 bits can be calculated using the data from
		the temperature, COUNT REMAIN and COUNT PER ?C registers in the
		scratchpad. Note that the COUNT PER ?C register is hard-wired to 16
		(10h). After reading the scratchpad, the TEMP_READ value is obtained
		by truncating the 0.5?C bit (bit 0) from the temperature data. The
		extended resolution temperature can then be calculated using the
		following equation:
		
		COUNT_PER_C - COUNT_REMAIN
		TEMPERATURE = TEMP_READ - 0.25 + --------------------------
		COUNT_PER_C
		*/
		
		// Good spot. Thanks Nic Johns for your contribution
		return (float)(rawTemperature >> 1) - 0.25 +((float)(scratchPad[COUNT_PER_C] - scratchPad[COUNT_REMAIN]) / (float)scratchPad[COUNT_PER_C] );
		//       break;
	}
	return 0.0; // error
}


// returns temperature in degrees F
// TODO: - when getTempC returns DEVICE_DISCONNECTED 
//        -127 gets converted to -196.6 F
float DallasTemperature_getTempF(uint8_t* deviceAddress)
{
	return DallasTemperature_toFahrenheit(DallasTemperature_getTempC(deviceAddress));
}

// returns TRUE if the bus requires parasite power
bool DallasTemperature_isParasitePowerMode(void)
{
	return parasite;
}

#if REQUIRESALARMS

/*

ALARMS:

TH and TL Register Format

BIT 7 BIT 6 BIT 5 BIT 4 BIT 3 BIT 2 BIT 1 BIT 0
S    2^6   2^5   2^4   2^3   2^2   2^1   2^0

Only bits 11 through 4 of the temperature register are used
in the TH and TL comparison since TH and TL are 8-bit
registers. If the measured temperature is lower than or equal
to TL or higher than or equal to TH, an alarm condition exists
and an alarm flag is set inside the DS18B20. This flag is
updated after every temperature measurement; therefore, if the
alarm condition goes away, the flag will be turned off after
the next temperature conversion.

*/

// sets the high alarm temperature for a device in degrees celsius
// accepts a float, but the alarm resolution will ignore anything
// after a decimal point.  valid range is -55C - 125C
void DallasTemperature_setHighAlarmTemp(uint8_t* deviceAddress, int8_t celsius)
{
	// make sure the alarm temperature is within the device's range
	if (celsius > 125) celsius = 125;
	else if (celsius < -55) celsius = -55;
	
	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad))
	{
		scratchPad[HIGH_ALARM_TEMP] = (uint8_t)celsius;
		writeScratchPad(deviceAddress, scratchPad);
	}
}

// sets the low alarm temperature for a device in degreed celsius
// accepts a float, but the alarm resolution will ignore anything
// after a decimal point.  valid range is -55C - 125C
void DallasTemperature_setLowAlarmTemp(uint8_t* deviceAddress, int8_t celsius)
{
	// make sure the alarm temperature is within the device's range
	if (celsius > 125) celsius = 125;
	else if (celsius < -55) celsius = -55;
	
	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad))
	{
		scratchPad[LOW_ALARM_TEMP] = (uint8_t)celsius;
		writeScratchPad(deviceAddress, scratchPad);
	}
}

// returns a char with the current high alarm temperature or
// DEVICE_DISCONNECTED for an address
int8_t DallasTemperature_getHighAlarmTemp(uint8_t* deviceAddress)
{
	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) return (int8_t)scratchPad[HIGH_ALARM_TEMP];
	return DEVICE_DISCONNECTED;
}

// returns a char with the current low alarm temperature or
// DEVICE_DISCONNECTED for an address
int8_t DallasTemperature_getLowAlarmTemp(uint8_t* deviceAddress)
{
	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) return (int8_t)scratchPad[LOW_ALARM_TEMP];
	return DEVICE_DISCONNECTED;
}

// resets internal variables used for the alarm search
void DallasTemperature_resetAlarmSearch()
{
	alarmSearchJunction = -1;
	alarmSearchExhausted = 0;
	for(uint8_t i = 0; i < 7; i++)
		alarmSearchAddress[i] = 0;
}

// This is a modified version of the OneWire_search method.
//
// Also added the OneWire search fix documented here:
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295
//
// Perform an alarm search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire_address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use
// DallasTemperature_resetAlarmSearch() to start over.
bool DallasTemperature_alarmSearch(uint8_t* newAddr)
{
	uint8_t i;
	int8_t lastJunction = -1;
	uint8_t done = 1;
	
	if (alarmSearchExhausted) return FALSE;
	if (!_wire->reset()) return FALSE;
	
	// send the alarm search command
	_wire->write(0xEC, 0);
	
	for(i = 0; i < 64; i++)
	{
		uint8_t a = _wire->read_bit( );
		uint8_t nota = _wire->read_bit( );
		uint8_t ibyte = i / 8;
		uint8_t ibit = 1 << (i & 7);
		
		// I don't think this should happen, this means nothing responded, but maybe if
		// something vanishes during the search it will come up.
		if (a && nota) return FALSE;
		
		if (!a && !nota)
		{
			if (i == alarmSearchJunction)
			{
				// this is our time to decide differently, we went zero last time, go one.
				a = 1;
				alarmSearchJunction = lastJunction;
			}
			else if (i < alarmSearchJunction)
			{
				// take whatever we took last time, look in address
				if (alarmSearchAddress[ibyte] & ibit) a = 1;
				else
				{
					// Only 0s count as pending junctions, we've already exhasuted the 0 side of 1s
					a = 0;
					done = 0;
					lastJunction = i;
				}
			}
			else
			{
				// we are blazing new tree, take the 0
				a = 0;
				alarmSearchJunction = i;
				done = 0;
			}
			// OneWire search fix
			// See: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295
		}
		
		if (a) alarmSearchAddress[ibyte] |= ibit;
		else alarmSearchAddress[ibyte] &= ~ibit;
		
		_wire->write_bit(a);
	}
	
	if (done) alarmSearchExhausted = 1;
	for (i = 0; i < 8; i++) newAddr[i] = alarmSearchAddress[i];
	return TRUE;
}

// returns TRUE if device address has an alarm condition
// TODO: can this be done with only TEMP_MSB REGISTER (faster)
//       if ((char) scratchPad[TEMP_MSB] <= (char) scratchPad[LOW_ALARM_TEMP]) return TRUE;
//       if ((char) scratchPad[TEMP_MSB] >= (char) scratchPad[HIGH_ALARM_TEMP]) return TRUE;
bool DallasTemperature_hasAlarm(uint8_t* deviceAddress)
{
	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad))
	{
		float temp = calculateTemperature(deviceAddress, scratchPad);
		
		// check low alarm
		if ((char)temp <= (char)scratchPad[LOW_ALARM_TEMP]) return TRUE;
		
		// check high alarm
		if ((char)temp >= (char)scratchPad[HIGH_ALARM_TEMP]) return TRUE;
	}
	
	// no alarm
	return FALSE;
}

// returns TRUE if any device is reporting an alarm condition on the bus
bool DallasTemperature_hasAlarm(void)
{
	DeviceAddress deviceAddress;
	resetAlarmSearch();
	return alarmSearch(deviceAddress);
}

// runs the alarm handler for all devices returned by alarmSearch()
void DallasTemperature_processAlarms(void)
{
	resetAlarmSearch();
	DeviceAddress alarmAddr;
	
	while (alarmSearch(alarmAddr))
	{
		if (validAddress(alarmAddr))
			_AlarmHandler(alarmAddr);
	}
}

// sets the alarm handler
void DallasTemperature_setAlarmHandler(AlarmHandler *handler)
{
	_AlarmHandler = handler;
}

// The default alarm handler
void DallasTemperature_defaultAlarmHandler(uint8_t* deviceAddress)
{
}

#endif

// Convert float fahrenheit to celsius
float DallasTemperature_toCelsius(float fahrenheit)
{
	return (fahrenheit - 32) / 1.8;
}

#if REQUIRESNEW

// MnetCS - Allocates memory for DallasTemperature. Allows us to instance a new object
void* DallasTemperature_operator new(unsigned int size) // Implicit NSS obj size
{
	void * p; // void pointer
	p = malloc(size); // Allocate memory
	memset((DallasTemperature*)p,0,size); // Initalise memory
	
	//!!! CANT EXPLICITLY CALL CONSTRUCTOR - workaround by using an init() methodR - workaround by using an init() method
	return (DallasTemperature*) p; // Cast blank region to NSS pointer
}

// MnetCS 2009 -  Unallocates the memory used by this instance
void DallasTemperature_operator delete(void* p)
{
	DallasTemperature* pNss =  (DallasTemperature*) p; // Cast to NSS pointer
	pNss->~DallasTemperature(); // Destruct the object
	
	free(p); // Free the memory
}

#endif


// initialise the bus
void DallasTemperature_begin(void)
{
	DeviceAddress deviceAddress;
	
	_wire->reset_search();
	devices = 0; // Reset the number of devices when we enumerate wire devices
	
	while (_wire->search(deviceAddress))
	{
		if (DallasTemperature_validAddress(deviceAddress))
		{
			if (!parasite && DallasTemperature_readPowerSupply(deviceAddress)) parasite = TRUE;
			
			ScratchPad scratchPad;
			
			DallasTemperature_readScratchPad(deviceAddress, scratchPad);
			
			bitResolution = max(bitResolution, DallasTemperature_getResolution2(deviceAddress));
			
			devices++;
		}
	}
}

struct DallasTemperature DS18B20 = 
{
  .Init = DallasTemperature_Init,
  .begin = DallasTemperature_begin,
  .getDeviceCount = DallasTemperature_getDeviceCount,
  .isConversionComplete = 0,
  .validAddress = DallasTemperature_validAddress,
  .getAddress = DallasTemperature_getAddress,
  .isConnected1 = DallasTemperature_isConnected1,
  .isConnected2 = DallasTemperature_isConnected2,
  .readScratchPad = DallasTemperature_readScratchPad,
  .writeScratchPad = DallasTemperature_writeScratchPad,
  .readPowerSupply = DallasTemperature_readPowerSupply,
  .getResolution1 = DallasTemperature_getResolution1,
  .setResolution1 = DallasTemperature_setResolution1,
  .getResolution2 = DallasTemperature_getResolution2,
  .setResolution2 = DallasTemperature_setResolution2,
  .setWaitForConversion = DallasTemperature_setWaitForConversion,
  .getWaitForConversion = DallasTemperature_getWaitForConversion,
  .setCheckForConversion = DallasTemperature_setCheckForConversion,
  .getCheckForConversion = DallasTemperature_getCheckForConversion,
  .requestTemperatures = DallasTemperature_requestTemperatures,
  .requestTemperaturesByAddress = DallasTemperature_requestTemperaturesByAddress,
  .requestTemperaturesByIndex = DallasTemperature_requestTemperaturesByIndex,
  .getTempC = DallasTemperature_getTempC,
  .getTempF = DallasTemperature_getTempF,
  .getTempCByIndex = DallasTemperature_getTempCByIndex,
  .getTempFByIndex = DallasTemperature_getTempFByIndex,
  .isParasitePowerMode = DallasTemperature_isParasitePowerMode,
  .isConversionAvailable = DallasTemperature_isConversionAvailable,
#if REQUIRESALARMS
  .setHighAlarmTemp = DallasTemperature_setHighAlarmTemp,
  .setLowAlarmTemp = DallasTemperature_setLowAlarmTemp,
  .getHighAlarmTemp = DallasTemperature_getHighAlarmTemp,
  .getLowAlarmTemp = DallasTemperature_getLowAlarmTemp,
  .resetAlarmSearch = DallasTemperature_resetAlarmSearch,
  .alarmSearch = DallasTemperature_alarmSearch,
  .hasAlarm1 = DallasTemperature_hasAlarm1,
  .hasAlarm2 = DallasTemperature_hasAlarm2,
  .processAlarms = DallasTemperature_processAlarms,
  .setAlarmHandler = DallasTemperature_setAlarmHandler,
  .defaultAlarmHandler = DallasTemperature_defaultAlarmHandler,
#endif
  .toFahrenheit = DallasTemperature_toFahrenheit,
  .toCelsius = DallasTemperature_toCelsius
};