/*
 * dac.c - dac setup 
 */
 
#include "dac.h"

/*
 * init all three DAC channels
 */
void dac_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
    DAC_HandleTypeDef DacHandle;
	DAC_ChannelConfTypeDef sConfig;
	
	/* enable clocks for DAC1, DAC2, GPIOA ----------------------------*/
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_DAC1_CLK_ENABLE();
	__HAL_RCC_DAC2_CLK_ENABLE();
	
	/* Configure analog output pins -----------------------------------*/
	GPIO_InitStructure.Pin =  GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
	GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* DAC1 channel 1&2 Configuration */
	DacHandle.Instance = DAC1;
	HAL_DAC_Init(&DacHandle);
	sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
	sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_1);
	HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_2);
	__HAL_DAC_ENABLE(&DacHandle, DAC_CHANNEL_1);
	__HAL_DAC_ENABLE(&DacHandle, DAC_CHANNEL_2);
    
	DacHandle.Instance = DAC2;
	HAL_DAC_Init(&DacHandle);
	HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_1);
	__HAL_DAC_ENABLE(&DacHandle, DAC_CHANNEL_1);
}

/*
 * set dac to output value
 */
void dac_set(uint8_t chl, uint16_t val)
{
    switch(chl)
    {
        case 0:
            DAC1->DHR12R1  = val;
            break;
        
        case 1:
            DAC1->DHR12R2  = val;
            break;
        
        case 2:
            DAC2->DHR12R1  = val;
            break;
    }
}
