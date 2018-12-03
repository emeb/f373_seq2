/*
 * cal.h - calibration routines for f373_seq2
 * (c) 2018 E. Brombaugh
 */

#ifndef __cal__
#define __cal__

#include "main.h"

void cal_update(cal_state *cs);
float32_t cal_raw2volt(uint16_t val, uint8_t chl);
uint16_t cal_volt2raw(float32_t val, uint8_t chl);
int16_t cal_raw2cent(uint16_t val, uint8_t chl);
uint16_t cal_cent2raw(int16_t val, uint8_t chl);
uint16_t cal_quant_step(uint16_t val, int8_t step, uint8_t chl);

#endif
