#ifndef ARGINFO_H
#define ARGINFO_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include "flash_layout.h"
#include "bl_uart.h"

bool arginfo_read(uint32_t *size,uint32_t *crc);

#endif
/* ARGINFO_H*/
