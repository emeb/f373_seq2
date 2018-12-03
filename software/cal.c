/*
 * cal.c - calibration routines for f373_seq2
 * (c) 2018 E. Brombaugh
 */

#include "cal.h"
#include "printf.h"

float32_t slope[SEQ_NUMCHLS], offset[SEQ_NUMCHLS];

/*
 * update float cal tables from cal struct
 */
void cal_update(cal_state *cs)
{
    uint8_t i;
    
    for(i=0;i<SEQ_NUMCHLS;i++)
    {
        slope[i] = -10.0F / ((float32_t)cs->cal_data[i][2]-(float32_t)cs->cal_data[i][0]);
        offset[i] = (float32_t)cs->cal_data[i][1];
    }
}

/*
 * convert raw DAC values to float volts
 */
float32_t cal_raw2volt(uint16_t val, uint8_t chl)
{
    return slope[chl]*((float32_t)val - offset[chl]);
}

/*
 * convert float volts to raw DAC value
 */
uint16_t cal_volt2raw(float32_t val, uint8_t chl)
{
    return (uint16_t)((val/slope[chl]) + offset[chl] + 0.5F);
}

/*
 * convert raw DAC values to cents
 */
int16_t cal_raw2cent(uint16_t val, uint8_t chl)
{
    return (int16_t)(1200.0F*cal_raw2volt(val, chl)+0.5F);
}

/*
 * convert cents to raw DAC value
 */
uint16_t cal_cent2raw(int16_t val, uint8_t chl)
{
    return cal_volt2raw((float32_t)val/1200.0F, chl);
}

/*
 * adjust raw value in 12ET quantized steps
 */
uint16_t cal_quant_step(uint16_t val, int8_t step, uint8_t chl)
{
    int16_t cents = cal_raw2cent(val, chl);
    int16_t quant = (cents+(cents<0?-50:50))/100;
    quant += step;
    cents = quant*100;
    return cal_cent2raw(cents, chl);
}
