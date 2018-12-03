/*
 * usart.c - usart diag support for F373 Seq2
 * 11-18-2018 E. Brombaugh
 */

#ifndef __usart__
#define __usart__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f3xx.h"

void setup_usart(void);
void usart_putc(void* p, char c);

#ifdef __cplusplus
}
#endif

#endif
