#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdbool.h>

struct ringbuffer;
typedef struct ringbuffer* ringbuffer_t;

ringbuffer_t rb8_new(uint8_t *data,uint32_t length);
bool rb8_puts(ringbuffer_t rb,uint8_t* data,uint32_t length);
bool rb8_gets(ringbuffer_t rb,uint8_t* data,uint32_t length);


#endif
/* RINGBUFFER_H*/
