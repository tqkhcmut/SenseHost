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


byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float celsius, fahrenheit;

void main(void)
{
	//  int i;
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	
	GPIO_Init(LED_RUN_PORT, LED_RUN_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
	Delay_Init();
	RS485_Init(115200);
	UART_Init(115200);
	GasLighting_Init();
	OneWire_Init();
	
	flash_read_buffer((char *)&flash_data, sizeof (struct flash_data));
	if (flash_data.id == BROADCAST_ID)
	{
		// get default id
		flash_data.id = DEV_MY_THESIS | 0x01;
	}
		
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
		
		if ( !OneWire_search(addr)) {
			UART_SendStr("\nNo more addresses.\n");
			UART_SendStr("\n\n");
			OneWire_reset_search();
			delay(250);
		}
		else
		{		
			UART_SendStr("ROM =");
			for( i = 0; i < 8; i++) {
				UART_SendChar(' ');
				UART_SendByte(addr[i], HEX);
			}
			
			if (OneWire_crc8(addr, 7) != addr[7]) {
				UART_SendStr("\nCRC is not valid!\n");
				goto skip;
			}
			UART_SendStr("\n\n");
			
			// the first ROM byte indicates which chip
			switch (addr[0]) {
			case 0x10:
				UART_SendStr("\n  Chip = DS18S20\n");  // or old DS1820
				type_s = 1;
				break;
			case 0x28:
				UART_SendStr("\n  Chip = DS18B20\n");
				type_s = 0;
				break;
			case 0x22:
				UART_SendStr("\n  Chip = DS1822\n");
				type_s = 0;
				break;
			default:
				UART_SendStr("\nDevice is not a DS18x20 family device.\n");
				goto skip;
			} 
			
			OneWire_reset();
			OneWire_select(addr);
			OneWire_write(0x44, 1);        // start conversion, with parasite power on at the end
			
			delay(1000);     // maybe 750ms is enough, maybe not
			// we might do a OneWire_depower() here, but the reset will take care of it.
			
			present = OneWire_reset();
			OneWire_select(addr);    
			OneWire_write(0xBE, 0);         // Read Scratchpad
			
			UART_SendStr("  Data = ");
			UART_SendByte(present, HEX);
			UART_SendStr(" ");
			for ( i = 0; i < 9; i++) {           // we need 9 bytes
				data[i] = OneWire_read();
				UART_SendByte(data[i], HEX);
				UART_SendStr(" ");
			}
			UART_SendStr(" CRC=");
			UART_SendByte(OneWire_crc8(data, 8), HEX);
			UART_SendStr("\n\n");
			
			// Convert the data to actual temperature
			// because the result is a 16 bit signed integer, it should
			// be stored to an "int16_t" type, which is always 16 bits
			// even when compiled on a 32 bit processor.
			int16_t raw = (data[1] << 8) | data[0];
			if (type_s) {
				raw = raw << 3; // 9 bit resolution default
				if (data[7] == 0x10) {
					// "count remain" gives full 12 bit resolution
					raw = (raw & 0xFFF0) + 12 - data[6];
				}
			} else {
				byte cfg = (data[4] & 0x60);
				// at lower res, the low bits are undefined, so let's zero them
				if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
				else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
				else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
				//// default is 12 bit resolution, 750 ms conversion time
			}
			celsius = (float)raw / 16.0;
			fahrenheit = celsius * 1.8 + 32.0;
			UART_SendStr("  Temperature = ");
			UART_SendFloat(celsius);
			UART_SendStr(" Celsius, ");
			UART_SendFloat(fahrenheit);
			UART_SendStr(" Fahrenheit\n");
		}
	skip:
		
		LED_RUN_TOGGLE;
		Delay(250);
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
