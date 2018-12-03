/*
 * outputs.h - output driver for F373 Seq2
 * 11-18-2018 E. Brombaugh
 */

#ifndef __outputs__
#define __outputs__

#include "main.h"

#define OUT_RST 0x01
#define OUT_TGA 0x02
#define OUT_TGB 0x04
#define OUT_TBC 0x08

void init_outputs(void);
void set_outputs_clk_bpm(uint16_t bpm);
void set_outputs_rst(uint8_t val);
void set_outputs_trig(uint8_t chl, uint8_t tg_typ, uint32_t per_ms);

#endif

 