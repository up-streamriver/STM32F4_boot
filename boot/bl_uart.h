#ifndef BL_UART_H
#define BL_UART_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>



typedef void (*bl_uart_recv_callback_t)(uint8_t *data, uint32_t len);
void bl_uart_recv_callback_register(bl_uart_recv_callback_t callback);
void bl_uart_init(void);
void bl_uart_deinit(void);
void bl_uart_write_data(uint8_t *data,uint32_t length);
// void bl_uart_write_string(const char* str);
// void bl_uart_printf(char *format,...);
#endif
/* BL_UART_H*/
