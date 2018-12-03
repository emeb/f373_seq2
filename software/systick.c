/*
 * systick.c - ad-ab-05 systick setup, plug, encoder and button polling 
 * E. Brombaugh 09-10-2016
 */

#include "systick.h"
#include "debounce.h"

#define REPEAT_DELAY 500
#define REPEAT_RATE 50

debounce_state systick_dbs[8];
uint8_t prev_acqcnt;
uint8_t event_hold;
uint32_t event_tick;
uint8_t rep_cnt;

void systick_init(void)
{
	uint8_t i;
    
	/* set up debounce objects for TSC sensors */
	for(i=0;i<TSC_NUMBTNS;i++)
    {
        init_debounce(&systick_dbs[i], 4);
    }
    
    prev_acqcnt = 0;
    event_hold = TSC_NUMBTNS;
    event_tick = 0;
}

/*
 * check state of TSC button
 */
uint8_t systick_get_button_state(uint8_t btn)
{
    /* valid button index? */
    if(btn<TSC_NUMBTNS)
        return systick_dbs[btn].state;
    else
        return 0;
}

/*
 * check rising edge of TSC button
 */
uint8_t systick_get_button_re(uint8_t btn)
{
    uint8_t result;
    
    /* valid button index? */
    if(btn<TSC_NUMBTNS)
    {
        /* get rising edge flag */
        result = systick_dbs[btn].re;
        systick_dbs[btn].re = 0;
        return result;
    }
    else
        return 0;
}

/*
 * get prioritized key event
 */
uint8_t systick_get_button_event(uint8_t *accel)
{
    uint8_t event=TSC_NUMBTNS;
    
    /* If no current key, look for new one */
    if(event_hold==TSC_NUMBTNS)
    {
        /* scan all buttons for new key */
        for(event=0;event<TSC_NUMBTNS;event++)
            if(systick_get_button_re(event))
                break;
        
        /* if new key found, record it */
        if(event!=TSC_NUMBTNS)
        {
            event_hold = event;
            event_tick = 0;
            rep_cnt = 0;
        }
    }
    else
    {
        /* handle repeat or key up */
        if(systick_dbs[event_hold].state)
        {
            if(event_tick > REPEAT_DELAY)
            {
                /* wait for delay time and then generate events at rate */
                event = event_hold;
                event_tick -= REPEAT_RATE;
                rep_cnt++;
            }
        }
        else
        {
            /* key up */
            event_hold = TSC_NUMBTNS;
            rep_cnt = 0;
        }
    }
    
    /* accelleration */
    if(rep_cnt < 10)
        *accel = 1;
    else if(rep_cnt < 25)
        *accel = 2;
    else if(rep_cnt < 50)
        *accel = 4;
    else if(rep_cnt < 75)
        *accel = 8;
    else 
        *accel = 16;
    
    return event;
}

/*
 * SysTick IRQ handler runs at 1000Hz, checks for plugs, buttons, etc.
 */
void SysTick_Handler(void)
{
    uint8_t i, curr_acqcnt;
    
	/* Needed by HAL! */
	HAL_IncTick();
    
	/* debounce encoder button */
	curr_acqcnt = TSC_GetState(TSC_NUMBTNS);
    if(curr_acqcnt != prev_acqcnt)
    {
        /* new TSC acquisition available */
        for(i=0;i<TSC_NUMBTNS;i++)
        {
            debounce(&systick_dbs[i], TSC_GetState(i));
        }
        prev_acqcnt = curr_acqcnt;
    }
    
    /* update key repeat timer */
    if(event_hold!=TSC_NUMBTNS)
        event_tick++;
}
