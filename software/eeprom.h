/*
 * eeprom.h - f373_seq2 eeprom interface routines
 */

#ifndef __eeprom__
#define __eeprom__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define EE_MAGIC_VAL 0xECE26B01
#define EE_ADDR_MAGIC 0
#define EE_LEN_MAGIC 4
#define EE_ADDR_CAL EE_LEN_MAGIC
#define EE_LEN_CAL 18
#define EE_ADDR_PATCH 64
#define EE_LEN_PATCH 192

void eeprom_init(void);
uint32_t eeprom_CheckReady(void);
uint32_t eeprom_WriteBuff(uint16_t WriteAddr, uint8_t *Data, uint16_t sz);
uint32_t eeprom_ReadBuff(uint16_t ReadAddr, uint8_t *Data, uint16_t sz);
void eeprom_get_cal(cal_state *cs);
void eeprom_set_cal(cal_state *cs);
void eeprom_get_patch(uint8_t patchnum, seq_state *ss);
void eeprom_set_patch(uint8_t patchnum, seq_state *ss);
void eeprom_default_patch(seq_state *ss);
void eeprom_clear(void);

#ifdef __cplusplus
}
#endif

#endif