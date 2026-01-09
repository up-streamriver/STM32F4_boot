#ifndef BUTTON_H
#define BUTTON_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>


void button_init(void);
bool button_is_pressed(void);
bool key_is_pressed(void);
#endif
/* BUTTON_H*/
