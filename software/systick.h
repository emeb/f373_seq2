/*
 * systick.h - f373_seq2 systick setup and TSC button polling 
 * E. Brombaugh 11-21-2018
 */

#ifndef __systick__
#define __systick__

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f3xx.h"
#include "tsc.h"

void systick_init(void);
uint8_t systick_get_button_state(uint8_t btn);
uint8_t systick_get_button_re(uint8_t btn);
uint8_t systick_get_button_event(uint8_t *accel);

#ifdef __cplusplus
}
#endif

#endif
