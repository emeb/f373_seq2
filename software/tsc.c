/*
 * tsc.c - F373 Seq2 touch setup
 */

#include "tsc.h"
#include "printf.h"

/*
 * Touch circuitry -
 * PA0  - cap TSC_G1_IO1
 * PA1  - TS1 TSC_G1_IO2 - 0 - up
 * PA2  - TS2 TSC_G1_IO3 - 1 - left
 * PA3  - TS3 TSC_G1_IO4 - 4 - +
 * PB3  - cap TSC_G5_IO1
 * PB4  - TS6 TSC_G5_IO2 - 7 - #
 * PB6  - TS7 TSC_G5_IO3 - 6 - *
 * PB7  - TS8 TSC_G5_IO4 - 2 - right
 * PB14 - cap TSC_G6_IO1
 * PB15 - TS4 TSC_G6_IO2 - 5 - -
 * PD8  - TS5 TSC_G6_IO3 - 3 - down
 */

/* TSC handler declaration */
TSC_HandleTypeDef TscHandle;

/* Array used to store the three acquisition value (one per channel) */
__IO uint32_t TSC_Value[TSC_NUMBTNS], TSC_Max[TSC_NUMBTNS];

uint8_t IdxBank, AcqCnt, AcqCal;
TSC_IOConfigTypeDef IoConfig;

/* Number of initial samples to estimate max */
#define TSC_ACQ_SMP_CNT 100

/*
 * hangup here for errors
 */
void TSC_Error_Handler(void)
{
    while(1)
    {
    }
}

/*
 * Initialize the breakout board LED
 */
void TSCInit(void)
{
    uint8_t i;
    GPIO_InitTypeDef GPIO_InitStruct;

    /*##-1- Enable TSC and GPIO clocks #########################################*/
    __HAL_RCC_TSC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*##-2- Configure Sampling Capacitor IOs (Alternate-Function Open-Drain) ###*/
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_TSC;
    GPIO_InitStruct.Pin       = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = GPIO_PIN_3 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    /*##-3- Configure Channel & Shield IOs (Alternate-Function Output PP) ######*/
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_TSC;
    GPIO_InitStruct.Pin       = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.Pin       = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*##-4- Configure the NVIC for TSC #########################################*/
    HAL_NVIC_SetPriority(EXTI2_TSC_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI2_TSC_IRQn);
    
    /*##-1- Configure the TSC peripheral #######################################*/
    TscHandle.Instance = TSC;
    TscHandle.Init.AcquisitionMode         = TSC_ACQ_MODE_NORMAL;
    TscHandle.Init.CTPulseHighLength       = TSC_CTPH_1CYCLE;
    TscHandle.Init.CTPulseLowLength        = TSC_CTPL_1CYCLE;
    TscHandle.Init.IODefaultMode           = TSC_IODEF_OUT_PP_LOW;
    TscHandle.Init.MaxCountInterrupt       = DISABLE;
    TscHandle.Init.MaxCountValue           = TSC_MCV_16383;
    TscHandle.Init.PulseGeneratorPrescaler = TSC_PG_PRESC_DIV32;
    TscHandle.Init.SpreadSpectrum          = DISABLE;
    TscHandle.Init.SpreadSpectrumDeviation = 127;
    TscHandle.Init.SpreadSpectrumPrescaler = TSC_SS_PRESC_DIV1;
    TscHandle.Init.SynchroPinPolarity      = TSC_SYNC_POLARITY_FALLING;
    TscHandle.Init.ChannelIOs              = 0; /* Not needed yet. Will be set with HAL_TSC_IOConfig() */
    TscHandle.Init.SamplingIOs             = 0; /* Not needed yet. Will be set with HAL_TSC_IOConfig() */
    TscHandle.Init.ShieldIOs               = 0; /* Not needed yet. Will be set with HAL_TSC_IOConfig() */

    if (HAL_TSC_Init(&TscHandle) != HAL_OK)
    {
        /* Initialization Error */
        TSC_Error_Handler();
    }

    /*##-2- Configure the touch-sensing IOs ####################################*/
    IoConfig.ChannelIOs  = TSC_GROUP1_IO2 | TSC_GROUP5_IO2 | TSC_GROUP6_IO2; /* Start with the first channel */
    //IoConfig.ChannelIOs  = TSC_GROUP1_IO2 | TSC_GROUP5_IO2; /* Start with the first channel */
    IoConfig.SamplingIOs =  TSC_GROUP1_IO1 | TSC_GROUP5_IO1 | TSC_GROUP6_IO1;
    if (HAL_TSC_IOConfig(&TscHandle, &IoConfig) != HAL_OK)
    {
        /* Initialization Error */
        TSC_Error_Handler();
    }

    /* init the bank select */
    IdxBank = 0;
    AcqCnt = 0;
    AcqCal = 1;
    for(i=0;i<TSC_NUMBTNS;i++)
    {
        TSC_Value[i] = 0;
        TSC_Max[i] = 0;
    }
    
    /*##-3- Discharge the touch-sensing IOs ####################################*/
    HAL_TSC_IODischarge(&TscHandle, ENABLE);
    HAL_Delay(1); /* 1 ms is more than enough to discharge all capacitors */

    /*##-4- Start the acquisition process ######################################*/
    if (HAL_TSC_Start_IT(&TscHandle) != HAL_OK)
    {
        /* Acquisition Error */
        TSC_Error_Handler();
    }
}

/*
 * get sense value
 */
uint32_t TSC_GetSense(uint8_t sense_num)
{
    if(sense_num < TSC_NUMBTNS)
    {
        if(!AcqCal)
            return TSC_Value[sense_num];
    }
    
    return 0;
}

/*
 * get thresholded state value
 */
uint8_t TSC_GetState(uint8_t sense_num)
{
    uint8_t result = 0;
    if(sense_num < TSC_NUMBTNS)
    {
        /* legal button index so get threshold state */
        if(!AcqCal)
            result = TSC_Value[sense_num] < (TSC_Max[sense_num]>>1);
    }
    else
        /* otherwise get count */
        result = AcqCnt;
    
    return result;
}

/**
  * @brief  Acquisition completed callback in non blocking mode 
  * @param  htsc: pointer to a TSC_HandleTypeDef structure that contains
  *         the configuration information for the specified TSC.
  * @retval None
  */
void HAL_TSC_ConvCpltCallback(TSC_HandleTypeDef* htsc)
{  
    uint8_t i;
    
    /*##-5- Discharge the touch-sensing IOs ####################################*/
    HAL_TSC_IODischarge(&TscHandle, ENABLE);

    /*##-8- Configure the next channels to be acquired #########################*/
    switch (IdxBank)
    {
        case 0:
			if (HAL_TSC_GroupGetStatus(&TscHandle, TSC_GROUP1_IDX) == TSC_GROUP_COMPLETED)
				TSC_Value[TSC_UP] = HAL_TSC_GroupGetValue(&TscHandle, TSC_GROUP1_IDX);  
			
			if (HAL_TSC_GroupGetStatus(&TscHandle, TSC_GROUP5_IDX) == TSC_GROUP_COMPLETED)
				TSC_Value[TSC_HASH] = HAL_TSC_GroupGetValue(&TscHandle, TSC_GROUP5_IDX);  
			
			if (HAL_TSC_GroupGetStatus(&TscHandle, TSC_GROUP6_IDX) == TSC_GROUP_COMPLETED)
				TSC_Value[TSC_MINUS] = HAL_TSC_GroupGetValue(&TscHandle, TSC_GROUP6_IDX);  

            IoConfig.ChannelIOs = TSC_GROUP1_IO3 | TSC_GROUP5_IO3 | TSC_GROUP6_IO3;
            IdxBank = 1;
            break;
        
        case 1:
			if (HAL_TSC_GroupGetStatus(&TscHandle, TSC_GROUP1_IDX) == TSC_GROUP_COMPLETED)
				TSC_Value[TSC_LEFT] = HAL_TSC_GroupGetValue(&TscHandle, TSC_GROUP1_IDX);  
			
			if (HAL_TSC_GroupGetStatus(&TscHandle, TSC_GROUP5_IDX) == TSC_GROUP_COMPLETED)
				TSC_Value[TSC_STAR] = HAL_TSC_GroupGetValue(&TscHandle, TSC_GROUP5_IDX);  
			
			if (HAL_TSC_GroupGetStatus(&TscHandle, TSC_GROUP6_IDX) == TSC_GROUP_COMPLETED)
				TSC_Value[TSC_DOWN] = HAL_TSC_GroupGetValue(&TscHandle, TSC_GROUP6_IDX);  

            IoConfig.ChannelIOs = TSC_GROUP1_IO4 | TSC_GROUP5_IO4;
            IdxBank = 2;
            break;
        
        case 2:
			if (HAL_TSC_GroupGetStatus(&TscHandle, TSC_GROUP1_IDX) == TSC_GROUP_COMPLETED)
				TSC_Value[TSC_PLUS] = HAL_TSC_GroupGetValue(&TscHandle, TSC_GROUP1_IDX);  
			
			if (HAL_TSC_GroupGetStatus(&TscHandle, TSC_GROUP5_IDX) == TSC_GROUP_COMPLETED)
				TSC_Value[TSC_RIGHT] = HAL_TSC_GroupGetValue(&TscHandle, TSC_GROUP5_IDX);  
			
            IoConfig.ChannelIOs = TSC_GROUP1_IO2 | TSC_GROUP5_IO2 | TSC_GROUP6_IO2;
            IdxBank = 0;
            
            /* Sample Max? */
            if(AcqCal)
            {
                /* Get Max */
                for(i=0;i<TSC_NUMBTNS;i++)
                    TSC_Max[i] = TSC_Max[i] > TSC_Value[i] ? TSC_Max[i] : TSC_Value[i];
                
                /* Done with calibration? */
                if(AcqCnt == TSC_ACQ_SMP_CNT)
                    AcqCal = 0;
            }
            
            /* done with all groups so updated count */
            AcqCnt++;
            
            break;

		default:
            break;
    }

    if (HAL_TSC_IOConfig(&TscHandle, &IoConfig) != HAL_OK)
    {
        /* Initialization Error */
        TSC_Error_Handler();
    }
    
    /*##-8- Re-start the acquisition process ###################################*/
    if (HAL_TSC_Start_IT(&TscHandle) != HAL_OK)
    {
        /* Acquisition Error */
        TSC_Error_Handler();
    }
}

#if 1
void HAL_TSC_ErrorCallback(TSC_HandleTypeDef* htsc)
{
	printf("TSC ERROR\n\r");
    while(1)
    {
    }
}
#endif

/**
  * @brief  This function handles EXTI2 and Touch Sensing Controller interrupt requests.
  * @param  None
  * @retval None
  */
void EXTI2_TSC_IRQHandler(void)
{
    HAL_TSC_IRQHandler(&TscHandle);
}

