/*
 * tsc.h - F373 Seq2 touch setup
 */

#ifndef __tsc__
#define __tsc__

#include "stm32f3xx.h"

#define TSC_NUMBTNS 8

enum tsc_btn
{
    TSC_UP,
    TSC_LEFT,
    TSC_RIGHT,
    TSC_DOWN,
    TSC_PLUS,
    TSC_MINUS,
    TSC_STAR,
    TSC_HASH,
};

void TSCInit(void);
uint32_t TSC_GetSense(uint8_t sense_num);
uint8_t TSC_GetState(uint8_t sense_num);

#endif
