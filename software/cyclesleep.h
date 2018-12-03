/*
 * cyclesleep.h - zyp's cycle counter sleep routines
 * 09-05-15 E. Brombaugh - updated for F7 and HAL
 * 03-20-17 E. Brombaugh - fixed wrap bug, update comments
 * 11-27-18 E. Brombaugh - port to F3, split delay to ms and us versions
 */

#ifndef __cyclesleep__
#define __cyclesleep__

#include "stm32f3xx.h"

void cyccnt_enable(void);
void cyclesleep(uint32_t cycles);
uint32_t cyclegoal(uint32_t cycles);
uint32_t cyclegoal_ms(uint32_t ms);
uint32_t cyclecheck(uint32_t goal);
void delayms(uint32_t ms);
void delayus(uint32_t us);
void start_meas(void);
void end_meas(void);
void get_meas(uint32_t *act, uint32_t *tot);

#endif
