/*
 * inputs.h - digital input driver for F373 Seq2
 * 11-23-2018 E. Brombaugh
 */

#ifndef __inputs__
#define __inputs__

#include "main.h"

extern uint8_t run_state, step_num[];

/* valid commands to input handler */
enum input_cmds
{
    INPUT_RUN,
    INPUT_STOP,
    INPUT_RESET,
    INPUT_FWD,
    INPUT_REV,
    INPUT_CLAMPSTEP,
    INPUT_INIT,
};

void init_inputs(void);
void input_set_state(uint8_t cmd);
uint8_t input_chg(void);
void input_update(void);

#endif
