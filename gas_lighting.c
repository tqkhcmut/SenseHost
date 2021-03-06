#include "gas_lighting.h"

__IO uint16_t Conversion_Value = 0;

int GasLighting_Init(void)
{
	/*  Init GPIO for ADC2 */
	GPIO_Init(GPIOE, GPIO_PIN_6, GPIO_MODE_IN_FL_NO_IT);
	GPIO_Init(GPIOE, GPIO_PIN_7, GPIO_MODE_IN_FL_NO_IT);
	
	/* De-Init ADC peripheral*/
	ADC2_DeInit();
	
	/* Init ADC2 peripheral */
	ADC2_Init(ADC2_CONVERSIONMODE_SINGLE, ADC2_CHANNEL_9, ADC2_PRESSEL_FCPU_D2, \
		ADC2_EXTTRIG_TIM, DISABLE, ADC2_ALIGN_RIGHT, ADC2_SCHMITTTRIG_CHANNEL9,\
			DISABLE);
	
	/*Start Conversion */
	ADC2_StartConversion();
	while (ADC2_GetFlagStatus() == RESET);
	Conversion_Value = ADC2_GetConversionValue();
	ADC2_ClearFlag();
	return 0;
}

float GasLighting_GetGas(void)
{
	/* De-Init ADC peripheral*/
	ADC2_DeInit();
	
	/* Init ADC2 peripheral */
	ADC2_Init(ADC2_CONVERSIONMODE_SINGLE, ADC2_CHANNEL_9, ADC2_PRESSEL_FCPU_D2, \
		ADC2_EXTTRIG_TIM, DISABLE, ADC2_ALIGN_RIGHT, ADC2_SCHMITTTRIG_CHANNEL9,\
			DISABLE);
	/*Start Conversion */
	ADC2_StartConversion();
	while (ADC2_GetFlagStatus() == RESET);
	Conversion_Value = ADC2_GetConversionValue();
	ADC2_ClearFlag();
//  res = Conversion_Value * 5 / 1024;
//  res = (5 - res) * 10000 / res;
//  res = 1024/Conversion_Value - 1;
	return 10240.0/Conversion_Value - 10.0;
}
float GasLighting_GetLighting(void)
{
	/* De-Init ADC peripheral*/
	ADC2_DeInit();
	
	/* Init ADC2 peripheral */
	ADC2_Init(ADC2_CONVERSIONMODE_SINGLE, ADC2_CHANNEL_8, ADC2_PRESSEL_FCPU_D2, \
		ADC2_EXTTRIG_TIM, DISABLE, ADC2_ALIGN_RIGHT, ADC2_SCHMITTTRIG_CHANNEL8,\
			DISABLE);
	
	/*Start Conversion */
	ADC2_StartConversion();
	while (ADC2_GetFlagStatus() == RESET);
	Conversion_Value = ADC2_GetConversionValue();
	ADC2_ClearFlag();
	
	// 1lux - 0.002V
	// 1000lux - 2v
	// x = 2.44141 * ADC value
	// RL = 10K
	return Conversion_Value * 2.44141;
}

/**
* @brief  ADC2 interrupt routine.
* @param  None
* @retval None
*/
INTERRUPT_HANDLER(ADC2_IRQHandler, 22)
{
	/* Get converted value */
	//    Conversion_Value = ADC2_GetConversionValue();
	
	ADC2_ClearITPendingBit();
}