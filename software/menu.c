/*
 * menu.h - f373_seq2_hal_base top-level menu management
 * E. Brombaugh
 */

#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "systick.h"
#include "printf.h"
#include "oled.h"
#include "inputs.h"
#include "outputs.h"
#include "eeprom.h"
#include "dac.h"
#include "cal.h"

#define MNU_X0_MAX (5+SEQ_NUMCHLS)
#define MNU_XN_MAX SEQ_NUMSTEPS
#define MNU_Y_MAX SEQ_NUMCHLS
#define MNU_FLAGS_STATIC 1

enum menu_types
{
    MNU_STATUS,
    MNU_CLOCK,
    MNU_RESET,
    MNU_ADD,
    MNU_LOAD,
    MNU_SAVE,
    MNU_CALIBRATE,
    MNU_SET,
    MNU_STEP,
};

const char *mnu_title[] =
{
    "Status",
    "Clock",
    "Reset",
    "Add",
    "Load",
    "Save",
    "Chl Cal",
    "Chl Set",
    "Step"
};

const char *rst_chl = "ABC-";

const char *add_names[] =
{
    "A    ", "A+B  ", "A+B+C", "     ",
    "B  ", "B+C"
};

const char *trg_type[] =
{
	"H0", "H1", "TP",
	"/1", "/2", "/4", "/8",
	"R2", "R3", "R4", "R8",
};

const char *seq_type[] =
{
    "Sq", "Rn"
};
    

const char *val_type[] =
{
	"Raw ",
	"Volt",
	"Cent",
	"Qnt "
};

int8_t mst_x, mst_y, mst_chg, mst_type, mst_chl, mst_item;
int8_t mst_lsptch, mst_currptch, mst_lsconf;
uint8_t eeprom_pending;
uint32_t eeprom_tickstart;
char textbuf[32];

/*
 * convert float value to si.fff formatted string
 */
void float2str(char *dest, float32_t val)
{
    int32_t is, ti, tf, neg=0;
    div_t qr;
    
    if(val<0)
    {
        neg = 1;
        val = -val;
    }
    is = val*100;
    qr = div(is, 100);
    ti = qr.quot;
    tf = qr.rem < 0 ? -qr.rem : qr.rem;
    sprintf(dest, "%c%1d.%02d", neg?'-':' ', ti, tf);
}

/*
 * draw menu page into a buffer
 */
void menu_render(uint8_t buf_num, uint8_t type, uint8_t flags)
{
    uint8_t i;
    
    /* Redrawing from scratch then do static content too */
	if(flags & MNU_FLAGS_STATIC)
    {
        /* wipe slate */
        oled_clear(buf_num, 0);
    
        /* Navigation cues */
        if(mst_x>0)
            oled_drawFastVLine(buf_num, 0, 0, 64, 1);
        if(mst_y==0)
        {
            if(mst_x<MNU_X0_MAX)
                oled_drawFastVLine(buf_num, 127, 0, 64, 1);
        }
        else
        {
            if(mst_x<MNU_XN_MAX)
                oled_drawFastVLine(buf_num, 127, 0, 64, 1);
        }
        if((mst_y>1)||((mst_y==1)&&(mst_x<=MNU_X0_MAX)))
            oled_drawFastHLine(buf_num, 0, 0, 128, 1);
        if(mst_y<MNU_Y_MAX)
            oled_drawFastHLine(buf_num, 0, 63, 128, 1);
        
        /* Header */
        sprintf(textbuf, "%s", mnu_title[type]);
        oled_drawstr(buf_num, 6, 2, textbuf, 1);
        if(type == MNU_CALIBRATE)
        {
            sprintf(textbuf, "%c", mst_chl+'A');
            oled_drawstr(buf_num, 2+9*8, 2, textbuf, 1);
        }
        else if((type == MNU_SET) || (type == MNU_STEP))
        {
            sprintf(textbuf, "%c", mst_chl+'A');
            oled_drawstr(buf_num, 2+9*8, 2, textbuf, 1);
        }
        if(type == MNU_STEP)
        {
            sprintf(textbuf, ":%2d", mst_x);
            oled_drawstr(buf_num, 2+10*8, 2, textbuf, 1);
        }
        oled_drawFastHLine(buf_num, 1, 11, 127, 1);
        
        /* Footer */
        oled_drawFastHLine(buf_num, 1, 52, 127, 1);
    }
    
    /* render per-type dynamic content */
    switch(type)
    {
        case MNU_STATUS:
            /* Channel Steps */
            for(i=0;i<SEQ_NUMCHLS;i++)
            {
                /* channel & step */
                sprintf(textbuf, "%c:%2d", 'A'+i, step_num[i]+1);
                oled_drawstr(buf_num, 2, 16+8*i, textbuf, 1);
                
                /* CV value */
                switch(seqst.chl_valtype[i])
                {
                    case VAL_RAW:
                        sprintf(textbuf, " %4d R", seqst.step_val[i][step_num[i]]);
                        break;
                    
                    case VAL_VOLTS:
                        float2str(textbuf, cal_raw2volt(seqst.step_val[i][step_num[i]], i));
                        strcat(textbuf, " V");
                        break;
                        
                    case VAL_CENTS:
                    case VAL_QUANT:
                        sprintf(textbuf, " %4d C", cal_raw2cent(seqst.step_val[i][step_num[i]], i));
                    break;
                }
                oled_drawstr(buf_num, 2+5*8, 16+8*i, textbuf, 1);
                
                /* Trigger type */
                sprintf(textbuf, "%s", trg_type[seqst.step_typ[i][step_num[i]]]);
                oled_drawstr(buf_num, 2+13*8, 16+8*i, textbuf, 1);
            }
            
            /* Key legends */
            oled_drawchar(buf_num,  2, 42, 27, 1);  // <-
            oled_drawchar(buf_num, 12, 42, '-', 0);
            oled_drawchar(buf_num, 22, 42, '+', 0);
            oled_drawchar(buf_num, 32, 42, 26, 1);  // <-
            
            oled_drawchar(buf_num, 44, 42, '*', 0);
            if(run_state)
                oled_drawchar(buf_num, 54, 42, 254, 1); // stop box
            else
                oled_drawchar(buf_num, 54, 42, 16, 1); // run >
            
            oled_drawchar(buf_num, 84, 42, '#', 0);
            oled_drawstr(buf_num, 94, 42, "Rst", 1);
            break;
        
        case MNU_CLOCK:
            /* draw clock bpm */
            oled_drawrect(buf_num, 6, 16, 54, 32, (mst_item==0)?1:0);
            sprintf(textbuf, "%3d", seqst.clock_bpm);
            oled_drawbitfont(buf_num, 6, 16, textbuf, (mst_item==0)?0:1);
			oled_drawstr(buf_num, 64, 32, "BPM", (mst_item==1)?0:1);
            break;
        
        case MNU_RESET:
            /* Reset Channel & Step */
            sprintf(textbuf, " Reset Chl:  %c", rst_chl[(seqst.rst>>4)]);
			oled_drawstr(buf_num, 8, 24, textbuf, (mst_item==0)?0:1);
            sprintf(textbuf, "Reset Step: %2d", (seqst.rst&0xf)+1);
            oled_drawstr(buf_num, 8, 32, textbuf, (mst_item==1)?0:1);
            break;
        
        case MNU_ADD:
            /* draw add options */
            sprintf(textbuf, "A : %s", add_names[seqst.add&3]);
			oled_drawstr(buf_num, 8, 24, textbuf, (mst_item==0)?0:1);
            sprintf(textbuf, "B : %s", add_names[4+((seqst.add>>2)&1)]);
			oled_drawstr(buf_num, 8, 32, textbuf, (mst_item==1)?0:1);
			oled_drawstr(buf_num, 8, 40, "C : C", 1);
            break;
        
        case MNU_LOAD:
        case MNU_SAVE:
            /* draw patch number */
            if((type==MNU_LOAD)&&(mst_lsptch==0))
				sprintf(textbuf, "Patch: Dflt", mst_lsptch);
			else
				sprintf(textbuf, "Patch: %2d  ", mst_lsptch);
			oled_drawstr(buf_num, 24, 24, textbuf, (mst_item==0)?0:1);
			
			/* draw confirm */
			if(!mst_lsconf)
				oled_drawstr(buf_num, 8, 32, "Confirm: ----", (mst_item==1)?0:1);
			else
			{
				if(type==MNU_LOAD)
					oled_drawstr(buf_num, 8, 32, "Confirm: Load", (mst_item==1)?0:1);
				else
					oled_drawstr(buf_num, 8, 32, "Confirm: Save", (mst_item==1)?0:1);
			}
            break;
        
        case MNU_CALIBRATE:
			sprintf(textbuf, "Calibrate CV %c", mst_chl+'A');
			oled_drawstr(buf_num, 6, 16, textbuf, 1);
			sprintf(textbuf, "+5.00V: %4d", calst.cal_data[mst_chl][0]);
			oled_drawstr(buf_num, 6, 24, textbuf, (mst_item==0)?0:1);
			sprintf(textbuf, " 0.00V: %4d", calst.cal_data[mst_chl][1]);
			oled_drawstr(buf_num, 6, 32, textbuf, (mst_item==1)?0:1);
			sprintf(textbuf, "-5.00V: %4d", calst.cal_data[mst_chl][2]);
			oled_drawstr(buf_num, 6, 40, textbuf, (mst_item==2)?0:1);
            break;
        
        case MNU_SET:
            /* draw steps */
            oled_drawrect(buf_num, 6, 16, 32, 32, (mst_item==0)?1:0);
            sprintf(textbuf, "%2d", seqst.chl_numsteps[mst_chl]);
            oled_drawbitfont(buf_num, 6, 16, textbuf, (mst_item==0)?0:1);
			oled_drawstr(buf_num, 42, 32, "Stp", 1);
        
            /* draw clkdiv */
			sprintf(textbuf, "Div:%2d", seqst.chl_clkdiv[mst_chl]);
			oled_drawstr(buf_num, 72, 24, textbuf, (mst_item==1)?0:1);
        
            /* draw seqtype */
            sprintf(textbuf, "Seq:%s", seq_type[seqst.chl_seqtype[mst_chl]]);
            oled_drawstr(buf_num, 72, 32, textbuf, (mst_item==2)?0:1);
        
            /* draw valtype */
            sprintf(textbuf, "Val: %c", *val_type[seqst.chl_valtype[mst_chl]]);
            oled_drawstr(buf_num, 72, 40, textbuf, (mst_item==3)?0:1);
            break;
        
        case MNU_STEP:
            /* draw value */
            oled_drawrect(buf_num, 6, 16, 82, 32, (mst_item==0)?1:0);
            switch(seqst.chl_valtype[mst_chl])
            {
                case VAL_RAW:
                    sprintf(textbuf, "%4d", seqst.step_val[mst_chl][mst_x-1]);
                    break;
                
                case VAL_VOLTS:
                    float2str(textbuf, cal_raw2volt(seqst.step_val[mst_chl][mst_x-1], mst_chl));
                    break;
                    
                case VAL_CENTS:
                case VAL_QUANT:
                    sprintf(textbuf, "%4d", cal_raw2cent(seqst.step_val[mst_chl][mst_x-1], mst_chl));
                break;
            }
            oled_drawbitfont(buf_num, 6, 16, textbuf, (mst_item==0)?0:1);
            oled_drawstr(buf_num, 88, 40, (char *)val_type[seqst.chl_valtype[mst_chl]], (mst_item==0)?0:1);
            
            /* draw trig type */
            sprintf(textbuf, "TG", trg_type[seqst.step_typ[mst_chl][mst_x-1]]);
            oled_drawstr(buf_num, 106, 16, textbuf, (mst_item==1)?0:1);
            sprintf(textbuf, "%s", trg_type[seqst.step_typ[mst_chl][mst_x-1]]);
            oled_drawstr(buf_num, 106, 24, textbuf, (mst_item==1)?0:1);
            break;
        
        default:
            /* nothing */
            oled_drawstr(buf_num, 48, 28, "Ack!", 1);
            break;
    }
	
	/* render generic dynamic content */
    /* Run/Stop status */
    if(run_state)
        oled_drawchar(buf_num, 2, 54, 16, 1); // run >
    else
        oled_drawchar(buf_num, 2, 54, 254, 1); // stop box

    /* Current patch */
    if(mst_currptch)
    {
        sprintf(textbuf, "%2d", mst_currptch);
        oled_drawstr(buf_num, 94, 54, textbuf, 1);
    }
    else
    {
        oled_drawstr(buf_num, 94, 54, "--", 1);
    }
    
    /* EEPROM status */
	if(eeprom_pending)
		oled_drawchar(buf_num, 118, 54, 7, 1);
	else
		oled_drawchar(buf_num, 118, 54, 9, 1);

}

void menu_init(void)
{
	/* initial menu location is off grid for splash */
	mst_x = -1;
	mst_y = 0;
	mst_chg = 1;
    mst_type = 0;
    mst_item = 0;
    eeprom_pending = 0;
	mst_lsptch = 0;
    mst_currptch = 0;
    
	/* load calibration & patch 0 into live seq state */
	eeprom_get_cal(&calst);
    cal_update(&calst);
    eeprom_get_patch(0, &seqst);
    set_outputs_clk_bpm(seqst.clock_bpm);
    input_update();
    
	/* draw splash */
	oled_drawstr(0, 0, 0, "   F373 Seq2", 1);
	oled_drawFastHLine(0, 0, 12, 128, 1);
	sprintf(textbuf, "V: %s", fwVersionStr);
	oled_drawstr(0, 0, 16, textbuf, 1);
	sprintf(textbuf, "D: %s", (char *)bdate);
	oled_drawstr(0, 0, 24, textbuf, 1);
	sprintf(textbuf, "T: %s", (char *)btime);
	oled_drawstr(0, 0, 32, textbuf, 1);
    oled_refresh(0);

}

void menu_update(void)
{
	uint8_t event, accel;
	int8_t nxt_x, nxt_y;
    uint8_t refresh = 0;
	
	/* next state same as current */
	nxt_x = mst_x;
	nxt_y = mst_y;
			
    /* get event event */
    event = systick_get_button_event(&accel);
    
	/* check for event update */
	if(!mst_chg)
	{
		/* normal operation */
		/* if navigation event then process */
		if(event<TSC_PLUS)
		{
            /* logic to prevent navigating to illegal row 0 location */
			if(((mst_y>1)||((mst_y==1)&&(mst_x<=MNU_X0_MAX)))&&(event==TSC_UP))
				nxt_y--;
			else if((mst_y<MNU_Y_MAX)&&(event==TSC_DOWN))
				nxt_y++;
			
			if((mst_x>0)&&(event==TSC_LEFT))
				nxt_x--;
			else if(mst_y==0)
            {
                if((mst_x<MNU_X0_MAX)&&(event==TSC_RIGHT))
                    nxt_x++;
            }
            else
            {
                if((mst_x<MNU_XN_MAX)&&(event==TSC_RIGHT))
                    nxt_x++;
            }
			
			if((mst_x!=nxt_x)||(mst_y!=nxt_y))
			{
				printf("%2d:%2d\n\r", nxt_x, nxt_y);
				mst_chg = 1;
                mst_item = 0;
			}
		}
	}
    else
    {
        /* initial transition from splash */
        nxt_x = nxt_y = 0;
    }
	
    /* transition or update */
	if(mst_chg)
	{
        /* transition to next menu page */
		uint8_t dir = OLED_RIGHT;
		
		/* determine direction */
		if(nxt_x<mst_x)
			dir = OLED_RIGHT;
		else if(nxt_x>mst_x)
			dir = OLED_LEFT;
		else if(nxt_y<mst_y)
			dir = OLED_DOWN;
		else if(nxt_y>mst_y)
			dir = OLED_UP;
		
		/* update state */
		mst_x = nxt_x;
		mst_y = nxt_y;
		mst_chg = 0;
        
        /* compute menu type from x/y location */
        if(mst_y==0)
        {
            /* top row is all control */
            if(mst_x==0)
                mst_type = MNU_STATUS;
            else if(mst_x==1)
                mst_type = MNU_CLOCK;
            else if(mst_x==2)
                mst_type = MNU_RESET;
            else if(mst_x==3)
                mst_type = MNU_ADD;
            else if(mst_x==4)
            {
                mst_type = MNU_LOAD;
                mst_lsptch = 0;
                mst_lsconf = 0;
            }
            else if(mst_x==5)
            {
                mst_type = MNU_SAVE;
                mst_lsptch = 1;
                mst_lsconf = 0;
            }
            else
            {
                mst_type = MNU_CALIBRATE;
                mst_chl = mst_x-6;
                input_set_state(INPUT_STOP);
                dac_set(mst_chl, calst.cal_data[mst_chl][0]);
            }
        }
        else
        {
            /* remaining rows are per-channel */
            if(mst_x==0)
                mst_type = MNU_SET;
            else
                mst_type = MNU_STEP;
            mst_chl = mst_y-1;
        }
        
        /* render new page to temp buffer */
		menu_render(1, mst_type, MNU_FLAGS_STATIC);
		
		/* transition */
		oled_slide(0, 1, 2, dir);
		
		/* update working buffer */
		oled_cpy_buf(0, 1);
	}
    else
    {
        /* check for parameter modification event */
        if((event>TSC_DOWN)&&(event<TSC_NUMBTNS))
        {
            /* handle parameters by menu type */
            switch(mst_type)
            {
                case MNU_STATUS:
                    if(event==TSC_PLUS)
                        input_set_state(INPUT_FWD);
                    
                    if(event==TSC_MINUS)
                        input_set_state(INPUT_REV);
                    
                    if(event==TSC_STAR)
                    {
                        if(run_state)
                            input_set_state(INPUT_STOP);
                        else
                            input_set_state(INPUT_RUN);
                    }
                    
                    if(event==TSC_HASH)
                        input_set_state(INPUT_RESET);
                    
                    input_update();
                    refresh = 1;
                    break;
                
                case MNU_CLOCK:
					/* update clock rate */
					if(event==TSC_PLUS)
					{
						if(seqst.clock_bpm<(999-accel))
							seqst.clock_bpm += accel;
						else
							seqst.clock_bpm = 999;
					}
					
					if(event==TSC_MINUS)
					{
						if(seqst.clock_bpm>(0+accel))
							seqst.clock_bpm-=accel;
						else
							seqst.clock_bpm = 0;
					}
					
					set_outputs_clk_bpm(seqst.clock_bpm);
                    mst_currptch = 0;
					eeprom_pending = 1;
					eeprom_tickstart = HAL_GetTick();
                    refresh = 1;
                    break;
                
                case MNU_RESET:
                    if(event==TSC_STAR)
                    {
                        if(mst_item>0)
                            mst_item--;
                    }
                    else if(event==TSC_HASH)
                    {
                        if(mst_item<1)
                            mst_item++;
                    }
                    else
                    {
                        uint8_t tmp;
                        
                        switch(mst_item)
                        {
							case 0: /* select reset chl */
                                tmp = seqst.rst>>4;
								if(event==TSC_PLUS)
								{
                                    if(tmp<3)
                                        tmp++;
								}
								
								if(event==TSC_MINUS)
								{
                                    if(tmp>0)
                                        tmp--;
								}
                                seqst.rst = (seqst.rst&0xf)|((tmp&3)<<4);
								break;
                            
							case 1: /* select reset step */
                                tmp = seqst.rst&0xf;
								if(event==TSC_PLUS)
								{
                                    if(tmp<15)
                                        tmp++;
								}
								
								if(event==TSC_MINUS)
								{
                                    if(tmp>0)
                                        tmp--;
								}
                                seqst.rst = (seqst.rst&0x30)|(tmp&0xf);
								break;
                        }
                        input_update();
                        eeprom_pending = 1;
                        eeprom_tickstart = HAL_GetTick();
                    }
                    refresh = 1;
                    break;
                
                case MNU_ADD:
                    if(event==TSC_STAR)
                    {
                        if(mst_item>0)
                            mst_item--;
                    }
                    else if(event==TSC_HASH)
                    {
                        if(mst_item<1)
                            mst_item++;
                    }
                    else
                    {
                        uint8_t tmp;
                        
                        switch(mst_item)
                        {
							case 0: /* select A add */
                                tmp = seqst.add&3;
								if(event==TSC_PLUS)
								{
                                    if(tmp<2)
                                        tmp++;
								}
								
								if(event==TSC_MINUS)
								{
                                    if(tmp>0)
                                        tmp--;
								}
                                seqst.add = (seqst.add&0x4)|(tmp&3);
								break;
                            
							case 1: /* select B add */
                                tmp = (seqst.add&0x4)>>2;
								if(event==TSC_PLUS)
								{
                                    if(tmp<1)
                                        tmp++;
								}
								
								if(event==TSC_MINUS)
								{
                                    if(tmp>0)
                                        tmp--;
								}
                                seqst.add = (seqst.add&0x3)|((tmp&1)<<2);
								break;
                        }
                        input_update();
                        eeprom_pending = 1;
                        eeprom_tickstart = HAL_GetTick();
                    }
                    refresh = 1;
                    break;
                
                case MNU_LOAD:
                case MNU_SAVE:
                    if(event==TSC_STAR)
                    {
                        if(mst_item>0)
                            mst_item--;
                    }
                    else if(event==TSC_HASH)
                    {
                        if(mst_item<1)
                            mst_item++;
                    }
                    else
                    {
                        switch(mst_item)
                        {
							case 0: /* select patch */
								if(event==TSC_PLUS)
								{
									if(mst_lsptch<16)
										mst_lsptch++;
								}
								
								if(event==TSC_MINUS)
								{
									if(mst_type==MNU_SAVE)
                                    {
                                        if(mst_lsptch>1)
                                            mst_lsptch--;
                                    }
                                    else
                                    {
                                        if(mst_lsptch>0)
                                            mst_lsptch--;
                                    }
								}
                                mst_lsconf = 0;
								break;
					
							case 1: /* confirm load/save selected patch */
								if(event==TSC_PLUS)
								{
									if(mst_type==MNU_SAVE)
                                    {
                                        printf("save %d\n\r", mst_lsptch);
                                        eeprom_set_patch(mst_lsptch, &seqst);
                                    }
                                    else
                                    {
                                        if(mst_lsptch)
                                        {
                                            printf("load %d\n\r", mst_lsptch);
                                            eeprom_get_patch(mst_lsptch, &seqst);
                                            mst_currptch = mst_lsptch;
                                        }
                                        else
                                        {
                                            printf("load default\n\r");
                                            eeprom_default_patch(&seqst);
                                            mst_currptch = 0;
                                        }
                                        set_outputs_clk_bpm(seqst.clock_bpm);
                                        input_set_state(INPUT_INIT);
                                        input_update();
                                        eeprom_pending = 1;
                                        eeprom_tickstart = HAL_GetTick();
                                    }
                                    mst_lsconf = 1;
								}
								break;
                        }
                    }
                    refresh = 1;
                    break;
                
                case MNU_CALIBRATE:
                    if(event==TSC_STAR)
                    {
                        if(mst_item>0)
                            mst_item--;
                    }
                    else if(event==TSC_HASH)
                    {
                        if(mst_item<2)
                            mst_item++;
                    }
                    else
                    {
						if(event==TSC_PLUS)
						{
							if(calst.cal_data[mst_chl][mst_item]<(4095-accel))
								calst.cal_data[mst_chl][mst_item] += accel;
							else
								calst.cal_data[mst_chl][mst_item] = 4095;
						}
						
						if(event==TSC_MINUS)
						{
							if(calst.cal_data[mst_chl][mst_item]>accel)
								calst.cal_data[mst_chl][mst_item] -= accel;
							else
								calst.cal_data[mst_chl][mst_item] = 0;
						}
                        cal_update(&calst);
						eeprom_pending = 2;
						eeprom_tickstart = HAL_GetTick();
                    }
                    dac_set(mst_chl, calst.cal_data[mst_chl][mst_item]);
                    refresh = 1;
                    break;
                
                case MNU_SET:
                    if(event==TSC_STAR)
                    {
                        if(mst_item>0)
                            mst_item--;
                    }
                    else if(event==TSC_HASH)
                    {
                        if(mst_item<3)
                            mst_item++;
                    }
                    else
                    {
                        switch(mst_item)
                        {
							case 0: /* update steps */
								if(event==TSC_PLUS)
								{
									if(seqst.chl_numsteps[mst_chl]<16)
										seqst.chl_numsteps[mst_chl] += 1;
								}
								
								if(event==TSC_MINUS)
								{
									if(seqst.chl_numsteps[mst_chl]>1)
										seqst.chl_numsteps[mst_chl] -= 1;
									
									/* adjust current steps if limit reduced */
									input_set_state(INPUT_CLAMPSTEP);
								}
								break;
							
							case 1: /* update divide ratio */
								if(event==TSC_PLUS)
								{
									if(seqst.chl_clkdiv[mst_chl]<16)
										seqst.chl_clkdiv[mst_chl]++;
								}
								
								if(event==TSC_MINUS)
								{
									if(seqst.chl_clkdiv[mst_chl]>1)
										seqst.chl_clkdiv[mst_chl]--;
								}
								break;
							
							case 2: /* update seq type */
								if(event==TSC_PLUS)
								{
									if(seqst.chl_seqtype[mst_chl]<1)
										seqst.chl_seqtype[mst_chl] += 1;
								}
								
								if(event==TSC_MINUS)
								{
									if(seqst.chl_seqtype[mst_chl]>0)
										seqst.chl_seqtype[mst_chl] -= 1;
								}
								break;
							
							case 3: /* update val type */
								if(event==TSC_PLUS)
								{
									if(seqst.chl_valtype[mst_chl]<3)
										seqst.chl_valtype[mst_chl] += 1;
								}
								
								if(event==TSC_MINUS)
								{
									if(seqst.chl_valtype[mst_chl]>0)
										seqst.chl_valtype[mst_chl] -= 1;
								}
								break;
								
						}
                        mst_currptch = 0;
						eeprom_pending = 1;
						eeprom_tickstart = HAL_GetTick();
                     }
					
                    input_update();
                    refresh = 1;
                    break;
                
                case MNU_STEP:
                    if(event==TSC_STAR)
                    {
                        if(mst_item>0)
                            mst_item--;
                    }
                    else if(event==TSC_HASH)
                    {
                        if(mst_item<1)
                            mst_item++;
                    }
                    else
                    {
                        switch(mst_item)
                        {
							case 0: /* update value */
                                {
                                    int16_t temp = seqst.step_val[mst_chl][mst_x-1];
                                    int16_t accel_s = accel;
                                    if(event==TSC_MINUS)
                                        accel_s = -accel_s;
                                    else if(event!=TSC_PLUS)
                                        accel_s = 0;
                                    if(seqst.chl_valtype[mst_chl]==VAL_QUANT)
                                    {
                                        /* quantized to 12ET */
                                        temp = cal_quant_step(temp, accel_s, mst_chl);
                                    }
                                    else
                                    {
                                        /* Unquantized */
                                        if(seqst.chl_valtype[mst_chl]!=VAL_RAW)
                                            accel_s = -accel_s;
                                        temp += accel_s;
                                    }
                                    temp = temp>4095?4095:temp;
                                    temp = temp<0?0:temp;
                                    seqst.step_val[mst_chl][mst_x-1]=temp;
                                }
                            break;
								
							case 1: /* update trigger type */
								if(event==TSC_PLUS)
								{
									if(seqst.step_typ[mst_chl][mst_x-1]<10)
										seqst.step_typ[mst_chl][mst_x-1] += 1;
								}
								
								if(event==TSC_MINUS)
								{
									if(seqst.step_typ[mst_chl][mst_x-1]>0)
										seqst.step_typ[mst_chl][mst_x-1] -= 1;
								}
								break;
						}
						
                        mst_currptch = 0;
						eeprom_pending = 1;
						eeprom_tickstart = HAL_GetTick();
					}
                    input_update();
                    refresh = 1;
                    break;
                
                default:
                    /* nothing */
                    break;
            }
        }
    }
	
	/* waiting for EEPROM update? */
	if(eeprom_pending)
	{
		if((HAL_GetTick() - eeprom_tickstart) > 5000)
		{
			/* State save? */
			if(eeprom_pending &1)
				eeprom_set_patch(0, &seqst);
			/* Cal save? */
			if(eeprom_pending &2)
				eeprom_set_cal(&calst);
			printf("EEPROM written\n\r");
			eeprom_pending = 0;
			refresh = 1;
		}
	}
	
    /* was there a change? */
    if(refresh||input_chg())
    {
        menu_render(0, mst_type, 0);
        oled_refresh(0);
    }
}

