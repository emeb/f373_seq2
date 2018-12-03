/*
 * eeprom.c - f373_seq2 eeprom interface routines
 */

#include "eeprom.h"
#include "printf.h"

#define EEPROM_ADDRESS (0x50<<1)
#define EEPROM_MAX_TRIALS ((uint32_t)3000)
#define EEPROM_TIMEOUT 1000

/* ST's crazy I2C timing register */
#define CODEC_I2Cx_TIMING ((uint32_t)0x2000090E)

/* local vars */
I2C_HandleTypeDef i2c_handler;

/*
 * Initialize the eeprom I2C bus
 */
void eeprom_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
    uint32_t magic;
    
	/* Enable I2C1 GPIO clock */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Hook up I2C1 to PB9/8 */
	GPIO_InitStructure.Pin =  GPIO_PIN_9|GPIO_PIN_8;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Enable the I2C1 peripheral clock & reset it */
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_I2C1_FORCE_RESET();
	__HAL_RCC_I2C1_RELEASE_RESET();
	
	/* I2C1 peripheral configuration */
	i2c_handler.Instance              = I2C1;
	i2c_handler.Init.Timing           = CODEC_I2Cx_TIMING;
	i2c_handler.Init.OwnAddress1      = 0;
	i2c_handler.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
	i2c_handler.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
	i2c_handler.Init.OwnAddress2      = 0;
	i2c_handler.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
	i2c_handler.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

	/* Init the I2C */
	HAL_I2C_Init(&i2c_handler);
    
    /* check if good settings in EEPROM */
    eeprom_ReadBuff(EE_ADDR_MAGIC, (uint8_t *)&magic, EE_LEN_MAGIC);
    printf("EEPROM Magic = 0x%8X\n\r", (unsigned int)magic);
    if(magic != EE_MAGIC_VAL)
    {
        /* initialize EEPROM */
        eeprom_clear();
        
        /* wait a bit and reset */
        printf("EEPROM defaults written - resetting\n\r");
        HAL_Delay(100);
        NVIC_SystemReset();
    }
    
}

/*
 * Check if the I2C bus is up and EEPROM is available 
 */
uint32_t eeprom_CheckReady(void)
{
	return HAL_I2C_IsDeviceReady(&i2c_handler, EEPROM_ADDRESS, EEPROM_MAX_TRIALS, EEPROM_TIMEOUT);
}

/*
 * Write a bunch of bytes
 */
uint32_t eeprom_WriteBuff(uint16_t WriteAddr, uint8_t *Data, uint16_t sz)
{
    uint16_t WriteSz;
    uint32_t status;
    
    /* handle multi-page writes */
    while(sz)
    {
        /* wait for setting write complete */
        while(eeprom_CheckReady()!=HAL_OK);
    
        printf("eeprom_WriteBuff: 0x%08X 0x%08X 0x%04X\n\r", (unsigned int)WriteAddr, (unsigned int)Data, (unsigned int)sz);
        
        /* write max 64 */
        WriteSz = (sz>64) ? 64 : sz;
        status = HAL_I2C_Mem_Write(&i2c_handler, EEPROM_ADDRESS, WriteAddr, I2C_MEMADD_SIZE_16BIT, Data, WriteSz, EEPROM_TIMEOUT);
        if(status != HAL_OK)
            return status;
        
        /* prep for next pass */
        sz = sz-WriteSz;
        WriteAddr += WriteSz;
        Data += WriteSz;
    }
    
    return status;
}

/*
 * Read a bunch of bytes
 */
uint32_t eeprom_ReadBuff(uint16_t ReadAddr, uint8_t *Data, uint16_t sz)
{	
    /* wait for setting write complete */
    while(eeprom_CheckReady()!=HAL_OK);
    
    printf("eeprom_ReadBuff: 0x%08X 0x%08X 0x%04X\n\r", (unsigned int)ReadAddr, (unsigned int)Data, (unsigned int)sz);
	return HAL_I2C_Mem_Read(&i2c_handler, EEPROM_ADDRESS, ReadAddr, I2C_MEMADD_SIZE_16BIT, Data, sz, EEPROM_TIMEOUT);
}

/*
 * Get cal data from the EEPROM
 */
void eeprom_get_cal(cal_state *cs)
{    
	eeprom_ReadBuff(EE_ADDR_CAL, (uint8_t *)cs, EE_LEN_CAL);
}

/*
 * Set cal data in EEPROM
 */
void eeprom_set_cal(cal_state *cs)
{
    eeprom_WriteBuff(EE_ADDR_CAL, (uint8_t *)cs, EE_LEN_CAL);
}

/* 
 * Get a patch from EEPROM
 */
void eeprom_get_patch(uint8_t patchnum, seq_state *ss)
{
	uint16_t addr = EE_ADDR_PATCH + patchnum * EE_LEN_PATCH;
	eeprom_ReadBuff(addr, (uint8_t *)ss, EE_LEN_PATCH);
}

/* Set a patch in EEPROM */
void eeprom_set_patch(uint8_t patchnum, seq_state *ss)
{
	uint16_t addr = EE_ADDR_PATCH + patchnum * EE_LEN_PATCH;
	eeprom_WriteBuff(addr, (uint8_t *)ss, EE_LEN_PATCH);
}

/* Get default patch */
void eeprom_default_patch(seq_state *ss)
{
	uint8_t i, j;
	ss->clock_bpm = 120;
	ss->rst = 0;
	ss->add = 0;
    for(i=0;i<SEQ_NUMCHLS;i++)
    {
        ss->chl_clkdiv[i] = 1;
        ss->chl_numsteps[i] = 16;
        ss->chl_seqtype[i] = 0;
        ss->chl_valtype[i] = 0;
        for(j=0;j<SEQ_NUMSTEPS;j++)
        {
            ss->step_val[i][j] = 2048;
            ss->step_typ[i][j] = 0;
        }
    }
    for(i=0;i<32;i++)
        ss->pad2[i] = 0xFF;
}

/*
 * Clear EEPROM to factory defaults
 */
void eeprom_clear(void)
{
	uint8_t i;
    uint32_t magic;
    cal_state cs;
	seq_state ss;
	
    /* Setup Magic */
    printf("eeprom_init: initializing magic\n\r");
    magic = EE_MAGIC_VAL;
    while(eeprom_CheckReady()!=HAL_OK);
    eeprom_WriteBuff(EE_ADDR_MAGIC, (uint8_t *)&magic, EE_LEN_MAGIC);
	
	/* Default Cal state */
    printf("eeprom_init: initializing cal\n\r");
	for(i=0;i<SEQ_NUMCHLS;i++)
	{
		cs.cal_data[i][0] = 584;    // Rough guess for +5V
		cs.cal_data[i][1] = 2048;   // Rough guess for 0V
		cs.cal_data[i][2] = 3513;   // Rough guess for -5V
	}
	eeprom_set_cal(&cs);
	
    /* default patches */
    printf("eeprom_init: initializing patches\n\r");
	eeprom_default_patch(&ss);
    for(i=0;i<17;i++)
    {
        eeprom_set_patch(i, &ss);
    }
}
