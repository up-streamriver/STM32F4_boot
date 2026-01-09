#ifndef BOARD_H
#define BOARD_H

#include "stm32f4xx.h"
#include <stdio.h>
#include "norflash.h"
#include "led.h"


void board_lowlevel_init(void);
void bl_lowlevel_deinit(void);
void bl_delay_init(void);
void bl_delay_ms(uint32_t ms);
uint32_t bl_now(void);


#endif
/* BOARD_H*/


