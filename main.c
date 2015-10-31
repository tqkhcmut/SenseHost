/**
******************************************************************************
* @file    Project/main.c 
* @author  MCD Application Team
* @version V2.2.0
* @date    30-September-2014
* @brief   Main program body
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
*
* Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
* You may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*        http://www.st.com/software_license_agreement_liberty_v2
*
* Unless required by applicable law or agreed to in writing, software 
* distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************************
*/ 


/* Includes ------------------------------------------------------------------*/
#include "stm8s.h"
#include "flash.h"
#include "delay.h"
#include "packet.h"
#include "rs485.h"
#include "gas_lighting.h"
#include "one_wire.h"
#include "DallasTemperature.h"
#include "uart.h"

/* Private defines -----------------------------------------------------------*/
#ifndef DEBUG
#define DEBUG	1
#endif
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
#define LED_RUN_PORT		GPIOD
#define LED_RUN_PIN			GPIO_PIN_0
#define LED_RUN_TOGGLE	{GPIO_WriteReverse(LED_RUN_PORT, LED_RUN_PIN);}

typedef unsigned char byte;

// arrays to hold device address
DeviceAddress insideThermometer;

// 
struct ThesisData mydata, * pdata;
struct flash_data flash_data;

#if !DEBUG
#define PACKET_BUFFER_SIZE 128
char packet_buff[PACKET_BUFFER_SIZE];
unsigned char packet_len;
struct Packet * packet;	
unsigned int tmp_time;
#endif

#if DEBUG
// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
	// method 2 - faster
	float tempC = DS18B20.getTempC(deviceAddress);
	UART_SendStr("Temp C: ");
	UART_SendFloat(tempC);
	UART_SendStr(" Temp F: ");
	UART_SendFloat(DS18B20.toFahrenheit(tempC)); // Converts tempC to Fahrenheit
	UART_SendStr("\n");
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		UART_SendByte(deviceAddress[i], HEX);
	}
}
#endif

void main(void)
{
	//  int i;
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	
	GPIO_Init(LED_RUN_PORT, LED_RUN_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
	Delay_Init();
	RS485_Init(115200);
	UART_Init(115200);
	GasLighting_Init();
	_one_wire.Init();
	DS18B20.Init(&_one_wire);
	DS18B20.begin();
	
	flash_read_buffer((char *)&flash_data, sizeof (struct flash_data));
	if (flash_data.id == BROADCAST_ID)
	{
		// get default id
		flash_data.id = DEV_MY_THESIS | 0x01;
	}
	
#if DEBUG
	UART_SendStr("Sensor Host.\n");
	
	// locate devices on the bus
	UART_SendStr("Locating devices...");
	UART_SendStr("Found ");
	UART_SendByte(DS18B20.getDeviceCount(), DEC);
	UART_SendStr(" devices.\n");
	
	// report parasite power requirements
	UART_SendStr("Parasite power is: "); 
	if (DS18B20.isParasitePowerMode())
		UART_SendStr("ON\n");
	else
		UART_SendStr("OFF\n");
	
	// assign address manually.  the addresses below will beed to be changed
	// to valid device addresses on your bus.  device address can be retrieved
	// by using either oneWire.search(deviceAddress) or individually via
	// sensors.getAddress(deviceAddress, index)
	//insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
	
	// Method 1:
	// search for devices on the bus and assign based on an index.  ideally,
	// you would do this to initially discover addresses on the bus and then 
	// use those addresses and manually assign them (see above) once you know 
	// the devices on your bus (and assuming they don't change).
	if (!DS18B20.getAddress(insideThermometer, 0)) 
		UART_SendStr("Unable to find address for Device 0\n"); 
	
	// method 2: search()
	// search() looks for the next device. Returns 1 if a new address has been
	// returned. A zero might mean that the bus is shorted, there are no devices, 
	// or you have already retrieved all of them.  It might be a good idea to 
	// check the CRC to make sure you didn't get garbage.  The order is 
	// deterministic. You will always get the same devices in the same order
	//
	// Must be called before search()
	//oneWire.reset_search();
	// assigns the first address found to insideThermometer
	//if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");
	
	// show the addresses we found on the bus
	UART_SendStr("Device 0 Address: ");
	printAddress(insideThermometer);
	UART_SendStr("\n");
	
	// set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
	DS18B20.setResolution2(insideThermometer, 9);
	
	UART_SendStr("Device 0 Resolution: ");
	UART_SendByte(DS18B20.getResolution2(insideThermometer), DEC); 
	UART_SendStr("\n");
#else
	// Must be called before search()
	_one_wire.reset_search();
	// assigns the first address found to insideThermometer
	if (!_one_wire.search(insideThermometer))
	{
		// unable to find address
	}
	// set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
	DS18B20.setResolution2(insideThermometer, 9);
#endif
	
	/* Infinite loop */
	while (1)
	{    
#if DEBUG
		UART_SendStr("\nGas Value: ");
		UART_SendFloat(GasLighting_GetGas());
		UART_SendStr(" kppm.\n");
		UART_SendStr("\nLighting Value: ");
		UART_SendFloat(GasLighting_GetLighting());
		UART_SendStr(" Lux.\n");
		
		// test on rs485 interface
		RS485_DIR_OUTPUT;
		RS485_SendStr("\nGas Value: ");
		RS485_SendFloat(GasLighting_GetGas());
		RS485_SendStr("\nLighting Value: ");
		RS485_SendFloat(GasLighting_GetLighting());
		RS485_SendStr(" Lux.");
		RS485_DIR_INPUT;
		
		// call sensors.requestTemperatures() to issue a global temperature 
		// request to all devices on the bus
		UART_SendStr("\nRequesting temperatures...");
		DS18B20.requestTemperatures(); // Send the command to get temperatures
		UART_SendStr("DONE\n");
		
		// It responds almost immediately. Let's print out the data
		printTemperature(insideThermometer); // Use a simple function to print out the data
		
		
		LED_RUN_TOGGLE;
		Delay(500);
#else
		if (Millis() - tmp_time > 500)
		{
			LED_RUN_TOGGLE;
			tmp_time = Millis();
			mydata.temperature = DS18B20.getTempC(insideThermometer);
			DS18B20.requestTemperatures(); // for the next time
			mydata.lighting = GasLighting_GetLighting();
			mydata.gas = GasLighting_GetGas();
		}
		
		if (RS485_Available() >= 8)
		{
			packet_len = RS485_GetData(packet_buff);
			packet = (struct Packet *)packet_buff;
			if (packet_len < 4 + getTypeLength(packet->data_type))
			{
				// not enough length
			}
			else
			{
				switch (packet->cmd)
				{
				case CMD_QUERY:
					if (packet->id == flash_data.id)
					{
						LED_RUN_TOGGLE;
						
						packet->data_type = TYPE_FLOAT | BIG_ENDIAN_BYTE_ORDER;
						pdata = (struct ThesisData *)packet->data;
						pdata->temperature = mydata.temperature;
						pdata->lighting = mydata.lighting;
						pdata->gas = mydata.gas;
						packet->data[getTypeLength(packet->data_type)] = checksum((char *)packet);
						RS485_DIR_OUTPUT;
						RS485_SendData(packet_buff, 4 + getTypeLength(packet->data_type));
						RS485_DIR_INPUT;
					}
					else if (IS_BROADCAST_ID(packet->id) && GPIO_ReadInputPin(RS485_SEL_PORT, RS485_SEL_PIN) == RESET)
					{
						// do the same above but check select pin is activited
						
						LED_RUN_TOGGLE;
						
						packet->id = flash_data.id;
						packet->data_type = TYPE_FLOAT | BIG_ENDIAN_BYTE_ORDER;
						pdata = (struct ThesisData *)packet->data;
						pdata->temperature = mydata.temperature;
						pdata->lighting = mydata.lighting;
						pdata->gas = mydata.gas;
						packet->data[getTypeLength(packet->data_type)] = checksum((char *)packet);
						RS485_DIR_OUTPUT;
						RS485_SendData(packet_buff, 4 + getTypeLength(packet->data_type));
						RS485_DIR_INPUT;
					}
					else
					{
						// not own id
					}
					break;
				case CMD_CONTROL:
					// for setting id
					break;
				default:
					break;
				}
				
				RS485_Flush();
			}
		}
#endif
	}
}

#ifdef USE_FULL_ASSERT

/**
* @brief  Reports the name of the source file and the source line number
*   where the assert_param error has occurred.
* @param file: pointer to the source file name
* @param line: assert_param error line source number
* @retval : None
*/
void assert_failed(u8* file, u32 line)
{ 
	/* User can add his own implementation to report the file name and line number,
	ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	
	/* Infinite loop */
	while (1)
	{
	}
}
#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
