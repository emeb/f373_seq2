/*
	main.c
	
	Part of f373_seq2_hal_base - stm32f373 sequencer + OLED + DAC + touch
	Copyright 11-18-2018 E. Brombaugh
*/

#include "main.h"
#include "printf.h"
#include "usart.h"
#include "systick.h"
#include "oled.h"
#include "outputs.h"
#include "inputs.h"
#include "dac.h"
#include "tsc.h"
#include "eeprom.h"
#include "menu.h"

/* build version in simple format */
const char *fwVersionStr = "0.1";
const char *bdate = __DATE__;
const char *btime = __TIME__;
seq_state seqst;
cal_state calst;

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 72000000
  *            HCLK(Hz)                       = 72000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            PLLMUL                         = RCC_PLL_MUL9 (9)
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  /* activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }
}

/*
 * Main routine
 */
int main(void)
{
	/* start up the HAL - note that this needs SysTick_Handler! */
	HAL_Init();
	
	/* Set clock source to HSE/PLL @ 72MHz */
	SystemClock_Config();
	
	/* init the UART for diagnostics */
	setup_usart();
	init_printf(0,usart_putc);
	printf("\n\n\rf373 Seq2 HAL\n\r");
	printf("\n");
	printf("SYSCLK = %d\n\r", HAL_RCC_GetSysClockFreq());
	printf("\n");
    
    printf("Seq State = %d bytes\n\r", sizeof(seq_state));
	
    /* initialize Systick button handler */
    systick_init();
	printf("Systick initialized\n\r");
    
    /* initialize cyclesleep fine timer */
    cyccnt_enable();
	printf("Cycle counter initialized\n\r");
    
    /* initialize OLED */
    oled_init();
	printf("OLED initialized\n\r");
    
    /* initialize the digital outs */
    init_outputs();
	printf("Outputs initialized\n\r");
    
    /* initialize the digital ins */
    init_inputs();
	printf("Inputs initialized\n\r");
    
	/* init the DACs */
    dac_init();
	printf("DACs initialized\n\r");

	/* start the touch sensor */
    TSCInit();
	printf("TSC initialized\n\r");
    
    /* enable EEPROM */
    eeprom_init();
    printf("EEPROM initialized\n\r");
	
    /* start menu */
	menu_init();
    printf("menu initialized\n\r");
	HAL_Delay(2000);
	
	printf("Looping...\n\r");
    while(1)
    {
		/* process the menu */
		menu_update();
		
		/* wait a bit */
        HAL_Delay(1);        
    }
}
