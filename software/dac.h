/*
 * dac.h - dac setup
 */

#ifndef __dac__
#define __dac__

#include "stm32f3xx.h"

void dac_init(void);
void dac_set(uint8_t chl, uint16_t val);

#endif
