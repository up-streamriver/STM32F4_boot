#ifndef LED_H
#define LED_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

void led_init(void);

void led_set(bool status);



#endif
/* LED_H*/
