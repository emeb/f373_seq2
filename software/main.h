/*
 * main.h - f373_seq2_hal_base top-level include
 * E. Brombaugh
 */

#ifndef __main__
#define __main__

#include <stdio.h>
#include <math.h>
#include "stm32f3xx.h"
#include "arm_math.h"
#include "cyclesleep.h"

#define SEQ_NUMCHLS 3
#define SEQ_NUMSTEPS 16

typedef struct
{
    uint16_t clock_bpm;
    uint8_t chl_clkdiv[SEQ_NUMCHLS];
    uint8_t chl_numsteps[SEQ_NUMCHLS];
    uint8_t chl_seqtype[SEQ_NUMCHLS];
    uint8_t chl_valtype[SEQ_NUMCHLS];
    uint8_t rst;
    uint8_t add;
    uint16_t step_val[SEQ_NUMCHLS][SEQ_NUMSTEPS];
    uint8_t step_typ[SEQ_NUMCHLS][SEQ_NUMSTEPS];
    uint8_t pad2[32];
} seq_state;

enum tg_types
{
    TG_H0,
    TG_H1,
    TG_TP,
    TG_WHOLE,
    TG_HALF,
    TG_QRTR,
    TG_EGTH,
    TG_RAT2,
    TG_RAT3,
    TG_RAT4,
    TG_RAT8,
};

enum value_types
{
    VAL_RAW,
    VAL_VOLTS,
    VAL_CENTS,
    VAL_QUANT,
};

typedef struct
{
	uint16_t cal_data[SEQ_NUMCHLS][3];
} cal_state;

extern const char *fwVersionStr;
extern const char *bdate;
extern const char *btime;
extern seq_state seqst;
extern cal_state calst;

#endif
