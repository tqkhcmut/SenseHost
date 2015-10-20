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

/* Private defines -----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
#define LED_RUN_PORT		GPIOD
#define LED_RUN_PIN			GPIO_PIN_0
#define LED_RUN_TOGGLE	{GPIO_WriteReverse(LED_RUN_PORT, LED_RUN_PIN);}

void main(void)
{
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
	
	GPIO_Init(LED_RUN_PORT, LED_RUN_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
	Delay_Init();
	RS485_Init(115200);
	GasLighting_Init();
		
	/* Infinite loop */
	while (1)
	{
		LED_RUN_TOGGLE;
		Delay(100);
//		RS485_SendNum(Conversion_Value);
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
