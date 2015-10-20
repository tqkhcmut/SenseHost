#include "gas_lighting.h"

__IO uint16_t Conversion_Value = 0;

int GasLighting_Init(void)
{
	/*  Init GPIO for ADC2 */
  GPIO_Init(GPIOE, GPIO_PIN_6, GPIO_MODE_IN_FL_NO_IT);
  
  /* De-Init ADC peripheral*/
  ADC2_DeInit();

  /* Init ADC2 peripheral */
  ADC2_Init(ADC2_CONVERSIONMODE_CONTINUOUS, ADC2_CHANNEL_9, ADC2_PRESSEL_FCPU_D2, \
            ADC2_EXTTRIG_TIM, DISABLE, ADC2_ALIGN_RIGHT, ADC2_SCHMITTTRIG_CHANNEL9,\
            DISABLE);
	
  /* Enable EOC interrupt */
  ADC2_ITConfig(ENABLE);

	enableInterrupts(); 
  
  /*Start Conversion */
  ADC2_StartConversion();
	return 0;
}

float GasLighting_GetGas(void)
{
	return Conversion_Value;
}
float GasLighting_GetLighting(void)
{
	return 0;
}

/**
  * @brief  ADC2 interrupt routine.
  * @param  None
  * @retval None
  */
 INTERRUPT_HANDLER(ADC2_IRQHandler, 22)
 {
	 /* Get converted value */
    Conversion_Value = ADC2_GetConversionValue();
		
		
    ADC2_ClearITPendingBit();
 }