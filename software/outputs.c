/*
 * outputs.c - output driver for F373 Seq2
 * 11-18-2018 E. Brombaugh
 */

#include "outputs.h"
#include "printf.h"

/*
 * Output mapping
 * clk o - PA15 TIM2_CH1
 * rst o - PF7  NA
 * a tg  - PB5  TIM3_CH2
 * b tg  - PA11 TIM4_CH1
 * c tg  - PA8  TIM5_CH1
 */

/* Map channels to timers */
TIM_TypeDef *TIM_Chl[] = {TIM3, TIM4, TIM5};
TIM_TypeDef *RepTIM_Chl[] = {TIM15, TIM16, TIM17};
uint8_t reps[3];
/*
 * init the outputs
 */
void init_outputs(void)
{
    uint8_t i;
	GPIO_InitTypeDef  GPIO_InitStructure;
    TIM_HandleTypeDef TimHandle;
	TIM_OC_InitTypeDef sConfig;
    TIM_OnePulse_InitTypeDef sConfig_OP;
    
    /* Clear repeat counts */
    for(i=0;i<3;i++)
        reps[i] = 0;
    
	/* Enable GPIO clocks */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
	
    /* PA15 set to TIM2 */
	GPIO_InitStructure.Pin = GPIO_PIN_15;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Alternate = GPIO_AF1_TIM2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Enable TIM2 for continuous run */
	TimHandle.Instance               = TIM2;
	TimHandle.Init.Prescaler         = 35999;	/* 2kHz timebase */
	TimHandle.Init.Period            = 999;	/* arbitrary 2Hz */
	TimHandle.Init.ClockDivision     = 0;
	TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	TimHandle.Init.RepetitionCounter = 0;
	TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_PWM_Init(&TimHandle);
    
	/* Enable TIM2 CH1 for clock pulse output*/
	sConfig.OCMode       = TIM_OCMODE_PWM1;
	sConfig.OCPolarity   = TIM_OCPOLARITY_LOW;
	sConfig.OCFastMode   = TIM_OCFAST_DISABLE;
	sConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
	sConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	sConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
	sConfig.Pulse        = 10;
	HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_1);
	
    /* TGA = PB5 set to TIM3_CH2 */
    __HAL_RCC_TIM3_CLK_ENABLE();
	GPIO_InitStructure.Pin = GPIO_PIN_5;
	GPIO_InitStructure.Alternate = GPIO_AF2_TIM3;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
    
	TimHandle.Instance               = TIM3;
	TimHandle.Init.Period            = 9;
	HAL_TIM_OnePulse_Init(&TimHandle, TIM_OPMODE_SINGLE);

	sConfig_OP.OCMode       = TIM_OCMODE_PWM2;
	sConfig_OP.OCPolarity   = TIM_OCPOLARITY_LOW;
	sConfig_OP.Pulse        = 1;
    sConfig_OP.ICPolarity   = TIM_ICPOLARITY_RISING;
    sConfig_OP.ICSelection  = TIM_ICSELECTION_DIRECTTI;
    sConfig_OP.ICFilter     = 0;
	HAL_TIM_OnePulse_ConfigChannel(&TimHandle, &sConfig_OP, TIM_CHANNEL_2, TIM_CHANNEL_1);
    TIM_CCxChannelCmd(TIM3, TIM_CHANNEL_2, TIM_CCx_ENABLE); 
    TIM3->BDTR|=(TIM_BDTR_MOE);
    
    /* TGB = PA11 set to TIM4_CH1 */
    __HAL_RCC_TIM4_CLK_ENABLE();
	GPIO_InitStructure.Pin = GPIO_PIN_11;
	GPIO_InitStructure.Alternate = GPIO_AF10_TIM4;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    
	TimHandle.Instance               = TIM4;
	HAL_TIM_OnePulse_Init(&TimHandle, TIM_OPMODE_SINGLE);

	HAL_TIM_OnePulse_ConfigChannel(&TimHandle, &sConfig_OP, TIM_CHANNEL_1, TIM_CHANNEL_2);
    TIM_CCxChannelCmd(TIM4, TIM_CHANNEL_1, TIM_CCx_ENABLE); 
    TIM4->BDTR|=(TIM_BDTR_MOE);

    /* TGC = PA8 set to TIM5_CH1 */
    __HAL_RCC_TIM5_CLK_ENABLE();
	GPIO_InitStructure.Pin = GPIO_PIN_8;
	GPIO_InitStructure.Alternate = GPIO_AF2_TIM5;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    
	TimHandle.Instance               = TIM5;
	HAL_TIM_OnePulse_Init(&TimHandle, TIM_OPMODE_SINGLE);
    
	HAL_TIM_OnePulse_ConfigChannel(&TimHandle, &sConfig_OP, TIM_CHANNEL_1, TIM_CHANNEL_2);
    TIM_CCxChannelCmd(TIM5, TIM_CHANNEL_1, TIM_CCx_ENABLE); 
    TIM5->BDTR|=(TIM_BDTR_MOE);

	/* RST = PF7 set to GP Output */
	GPIO_InitStructure.Pin =  GPIO_PIN_7;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStructure);
    
    /* TIM15 is repeat interval for TG A ratchet */
    __HAL_RCC_TIM15_CLK_ENABLE();
	TimHandle.Instance               = TIM15;
	TimHandle.Init.Period            = 9;
	HAL_TIM_OnePulse_Init(&TimHandle, TIM_OPMODE_SINGLE);
    TIM15->DIER |= TIM_DIER_UIE;

    HAL_NVIC_SetPriority(TIM15_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(TIM15_IRQn);

    /* TIM16 is repeat interval for TG B ratchet */
    __HAL_RCC_TIM16_CLK_ENABLE();
	TimHandle.Instance               = TIM16;
	TimHandle.Init.Period            = 9;
	HAL_TIM_OnePulse_Init(&TimHandle, TIM_OPMODE_SINGLE);
    TIM16->DIER |= TIM_DIER_UIE;
    
    HAL_NVIC_SetPriority(TIM16_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(TIM16_IRQn);

    /* TIM17 is repeat interval for TG C ratchet */
    __HAL_RCC_TIM17_CLK_ENABLE();
	TimHandle.Instance               = TIM17;
	TimHandle.Init.Period            = 9;
	HAL_TIM_OnePulse_Init(&TimHandle, TIM_OPMODE_SINGLE);
    TIM17->DIER |= TIM_DIER_UIE;

    HAL_NVIC_SetPriority(TIM17_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(TIM17_IRQn);
}

/*
 * set clock BPM
 */
void set_outputs_clk_bpm(uint16_t bpm)
{
    /* convert from BPM to timer units */
    uint32_t Period = (1000*120/(uint32_t)bpm)-1;
    TIM2->ARR = Period;
	if(TIM2->CNT > Period)
		TIM2->CNT = 0;
}

/*
 * drive timers with trigger types
 */
void set_outputs_trig(uint8_t chl, uint8_t tg_typ, uint32_t per_ms)
{
    uint8_t rep=1;
    
    /* scale up for 500us counter rate */
    per_ms<<=1;
    
    /* select pulse type */
    switch(tg_typ)
    {
        case TG_H0: /* Hold 0 : Force inactive (OCM 100) */
            if(chl==0)
                TIM_Chl[chl]->CCMR1 = (TIM_Chl[chl]->CCMR1 & ~(TIM_CCMR1_OC2M)) | TIM_CCMR1_OC2M_2;
            else
                TIM_Chl[chl]->CCMR1 = (TIM_Chl[chl]->CCMR1 & ~(TIM_CCMR1_OC1M)) | TIM_CCMR1_OC1M_2;
            break;
            
        case TG_H1: /* Hold 1 : Force active (OCM 101) */
            if(chl==0)
                TIM_Chl[chl]->CCMR1 = (TIM_Chl[chl]->CCMR1 & ~(TIM_CCMR1_OC2M)) | TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_2;
            else
                TIM_Chl[chl]->CCMR1 = (TIM_Chl[chl]->CCMR1 & ~(TIM_CCMR1_OC1M)) | TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_2;
            break;
        
            /*
             * Ratchets are min-width pulses that
             * use additional timer to generate repeats
             */
        case TG_RAT8: /* Eight pulses (seven repeats @ per_ms/8) */
            rep=5;
        case TG_RAT4: /* Four pulses (three repeats @ per_ms/4) */
            rep++;
        case TG_RAT3: /* Three pulses (two repeats @ per_ms/3) */
            rep++;
        case TG_RAT2: /* Two pulses (one repeat @ per_ms/2) */
            /* compute repeat period */ 
            switch(rep)
            {
                case 1: per_ms>>=1; break;
                case 2: per_ms/=3; break;
                case 3: per_ms>>=2; break;
                case 7: per_ms>>=3; break;
            }
            reps[chl] = rep;
            RepTIM_Chl[chl]->ARR = per_ms;
            RepTIM_Chl[chl]->CR1 |= TIM_CR1_CEN;
            
            /* 
             * Pulses : PWM mode 2 (OCM 110)
             * These all fall through to compute fractions.
             */
        case TG_TP: /* min width trigger pulse */
            per_ms = 160;
        case TG_EGTH: /* Period/8 pulse */
            per_ms>>=1;
        case TG_QRTR: /* Period/4 pulse */
            per_ms>>=1;
        case TG_HALF: /* Period/2 pulse */
            per_ms>>=1;
        case TG_WHOLE: /* Period pulse */
            if(chl==0)
            {
                TIM_Chl[chl]->CCMR1 = (TIM_Chl[chl]->CCMR1 & ~(TIM_CCMR1_OC2M)) | TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
            }
            else
            {
                TIM_Chl[chl]->CCMR1 = (TIM_Chl[chl]->CCMR1 & ~(TIM_CCMR1_OC1M)) | TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
            }
            TIM_Chl[chl]->ARR = per_ms-10; // slightly short to allow drop @ end
            TIM_Chl[chl]->CNT = 1;
            TIM_Chl[chl]->CR1 |= TIM_CR1_CEN;
            break;
    }
}

/*
 * force value on RST output 
 */
void set_outputs_rst(uint8_t val)
{
    if(val&1)
        GPIOF->ODR &= ~(1<<7);
    else
        GPIOF->ODR |= (1<<7);
}

/*
 * common ISR core logic for repeats
 */
void repeat_isr(uint8_t chl)
{
    if((RepTIM_Chl[chl]->SR & TIM_FLAG_UPDATE)==TIM_FLAG_UPDATE)
    {
        RepTIM_Chl[chl]->SR = ~TIM_FLAG_UPDATE;
        
        /* repeats left to do? */
        if(reps[chl])
        {
            /* dec repeat count */
            reps[chl]--;
            
            /* start new trigger pulse on TG A timer */
            TIM_Chl[chl]->CNT = 1;
            TIM_Chl[chl]->CR1 |= TIM_CR1_CEN;
            
            /* start new repeat interval */
            RepTIM_Chl[chl]->CR1 |= TIM_CR1_CEN;
        }
    }
}

/*
 * ISR for TG A repetitions
 */
void TIM15_IRQHandler(void)
{
    repeat_isr(0);
}

/*
 * ISR for TG B repetitions
 */
void TIM16_IRQHandler(void)
{
    repeat_isr(1);
}

/*
 * ISR for TG C repetitions
 */
void TIM17_IRQHandler(void)
{
    repeat_isr(2);
}
