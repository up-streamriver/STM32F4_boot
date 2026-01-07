#include "main.h"

#define bl_uart_buffer_size 4096

static uint8_t bl_uart_buffer[bl_uart_buffer_size];
ringbuffer_t serial_rx;

extern void boot_application(void);

void bl_uart_recv_temp_store(uint8_t data)
{
	rb8_puts(serial_rx,&data,1);
}

int main(void)
{
	 extern JUMP_APP(uint32_t base);
	// JUMP_APP(0x8010000);
	
	board_lowlevel_init();
	bl_uart_init();
	serial_rx = rb8_new(bl_uart_buffer,bl_uart_buffer_size);
	bl_uart_recv_callback_register(bl_uart_recv_temp_store);
	bl_uart_write_string("initial success\r\n");
	bl_uart_printf("lalalalalla");

	
	while(1)
	{	
		uint8_t data;
		if(rb8_gets(serial_rx,&data,1))
		{
			if(data == 0x05)
				boot_application();
			else
			bl_uart_printf("Data:%02x\r\n",data);
		}
			

 
	}
	
	return 0;
}
