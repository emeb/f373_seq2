/*
 * inputs.c - digital input driver for F373 Seq2
 * 11-23-2018 E. Brombaugh
 */

#include <stdlib.h>
#include "inputs.h"
#include "outputs.h"
#include "dac.h"
#include "cal.h"

/*
 * Input mapping
 * clk i - PC13
 * rst i - PC14
 */
 
uint8_t run_state, change, step_num[SEQ_NUMCHLS], clk_cnt[SEQ_NUMCHLS],
    trig_step[SEQ_NUMCHLS], seqtype_state[SEQ_NUMCHLS];
uint32_t prev_tick, clk_per;

/*
 * initialize input handler
 */
void init_inputs(void)
{
    uint8_t i;
	GPIO_InitTypeDef GPIO_InitStructure;

    /* Turn on GPIO C clock */
	__HAL_RCC_GPIOC_CLK_ENABLE();
    
    /* Set PC13, PC14 as EXTI falling w/ pullup */
	GPIO_InitStructure.Pin =  GPIO_PIN_13 | GPIO_PIN_14;
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* turn on EXTI interrupts */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    
    /* initially stopped at first step */
    run_state = 0;
    change = 0;
    for(i=0;i<SEQ_NUMCHLS;i++)
    {
        step_num[i] = 0;
        clk_cnt[i] = 0;
        seqtype_state[i] = 0;
    }
    clk_per = 1000;
    prev_tick = 0;
}

/*
 * update input handler state atomically
 */
void input_set_state(uint8_t cmd)
{
    uint8_t i;
    
    /* take the lock */
    __disable_irq();

    /* execute command */
    switch(cmd)
    {
        case INPUT_RUN:
            if(!run_state)
                run_state = 1;
            break;
        
        case INPUT_STOP:
            if(run_state)
                run_state = 0;
            break;
        
        case INPUT_RESET:
            for(i=0;i<SEQ_NUMCHLS;i++)
            {
                step_num[i] = 0;
                clk_cnt[i] = 0;
            }
            break;
        
        case INPUT_FWD:
            for(i=0;i<SEQ_NUMCHLS;i++)
            {
                step_num[i] = (step_num[i]+1)%seqst.chl_numsteps[i];
            }
            break;
        
        case INPUT_REV:
            for(i=0;i<SEQ_NUMCHLS;i++)
            {
                if(step_num[i]>0)
                    step_num[i] = step_num[i]-1;
                else
                    step_num[i] = seqst.chl_numsteps[i]-1;
            }
            break;
            
         case INPUT_CLAMPSTEP:
            for(i=0;i<SEQ_NUMCHLS;i++)
            {
                if(step_num[i]>=seqst.chl_numsteps[i])
                    step_num[i] = seqst.chl_numsteps[i]-1;
            }
            break;
            
         case INPUT_INIT:
            run_state = 0;
            change = 0;
            for(i=0;i<SEQ_NUMCHLS;i++)
            {
                step_num[i] = 0;
                clk_cnt[i] = 0;
            }
            break;
   }
    
    /* release the lock */
    __enable_irq();
}

/*
 * report if input state changed since last check
 */
uint8_t input_chg(void)
{
    uint8_t result = change;
    change = 0;
    return result;
}

/*
 * Update outputs due to state change
 */
void input_update(void)
{
    uint8_t i;
    float32_t volts;
    
    /* set DAC A */
    switch(seqst.add&3)
    {
        case 0: /* no add */
        default: /* illegal selection */
            dac_set(0, seqst.step_val[0][step_num[0]]);
            break;
        
        case 1: /* A+B */
            /* convert A & B to volts & add */
            volts = cal_raw2volt(seqst.step_val[0][step_num[0]], 0);
            volts += cal_raw2volt(seqst.step_val[1][step_num[1]], 1);
            dac_set(0, cal_volt2raw(volts, 0));
            break;
        
        case 2: /* A+B+C */
            /* convert A & B to volts & add */
            volts = cal_raw2volt(seqst.step_val[0][step_num[0]], 0);
            volts += cal_raw2volt(seqst.step_val[1][step_num[1]], 1);
            volts += cal_raw2volt(seqst.step_val[2][step_num[2]], 2);
            dac_set(0, cal_volt2raw(volts, 0));
            break;
    }
    
    /* set DAC B */
    if(seqst.add&4)
    {
        /* convert B & C to volts & add */
        volts = cal_raw2volt(seqst.step_val[1][step_num[1]], 1);
        volts += cal_raw2volt(seqst.step_val[2][step_num[2]], 2);
        dac_set(1, cal_volt2raw(volts, 1));
    }
    else
        dac_set(1, seqst.step_val[1][step_num[1]]);
    
    /* set DAC C - no adding */
    dac_set(2, seqst.step_val[2][step_num[2]]);
    
    /* delay 50us for DAC slew */
    delayus(50);
    
    /* handle reset */
    set_outputs_rst(((seqst.rst>>4)!=3) && (step_num[seqst.rst>>4]==(seqst.rst&0xf)));
    
    /* handle trigger/gates */
    for(i=0;i<SEQ_NUMCHLS;i++)
    {
        if(trig_step[i])
        {
            set_outputs_trig(i, seqst.step_typ[i][step_num[i]], seqst.chl_clkdiv[i]*clk_per);
        }
    }
}

/*
 * IRQ handler for EXTI 10-15
 */
void EXTI15_10_IRQHandler(void)
{
    uint8_t i;
    uint32_t curr_tick;
    
    /* compute clock period */
    curr_tick = HAL_GetTick();
    clk_per = curr_tick-prev_tick;
    prev_tick = curr_tick;
    
    /* clock input */
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13) != RESET)
    {
        if(run_state)
        {
            /* advance step */
            for(i=0;i<SEQ_NUMCHLS;i++)
            {
                clk_cnt[i]++;
                if(clk_cnt[i]==seqst.chl_clkdiv[i])
                {
                    trig_step[i] = 1;
                    clk_cnt[i] = 0;
                    if(seqst.chl_seqtype[i]==0)
                    {
                        /* sequential */
                        step_num[i] = (step_num[i]+1)%seqst.chl_numsteps[i];
                    }
                    else
                    {
                        /* random */
                        step_num[i] = rand()%seqst.chl_numsteps[i];
                    }
                }
                else
                    trig_step[i] = 0;
                    
            }
            
            input_update();
            change = 1;
        }
        
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
    }
    
    /* reset input */
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14) != RESET)
    {
        /* reset all channels to step 0 */
        for(i=0;i<SEQ_NUMCHLS;i++)
        {
            step_num[i] = 0;
            clk_cnt[i] = 0;
        }
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_14);
    }
}
