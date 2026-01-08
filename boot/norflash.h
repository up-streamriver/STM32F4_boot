#ifndef NORFLASH_H
#define NORFLASH_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include "flash_layout.h"
#include "bl_uart.h"


void bl_flash_lock(void);
void bl_flash_unlock(void);
void bl_flash_erase(uint32_t address,uint32_t size);
void bl_flash_write_word(uint32_t address,uint8_t *data,uint32_t size);
#endif
/* NORFLASH_H*/
