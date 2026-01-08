#include "main.h"

void test(void)
{
	uint8_t head = 0xAA;
	uint8_t op = 0x1F;
	uint16_t length = 0x00;
	uint8_t data = 0x02;
	uint32_t crc = 0;
	crc = crc32_update(crc,(uint8_t*)&head,1);
	crc = crc32_update(crc,(uint8_t*)&op,1);
	crc = crc32_update(crc,(uint8_t*)&length,2);
	//crc = crc32_update(crc,(uint8_t*)&data,1);

	bl_uart_printf("crc : %08x",crc);
}

int main(void)
{
	 extern JUMP_APP(uint32_t base);
	// JUMP_APP(0x8010000);
	
	board_lowlevel_init();
	bl_uart_init();
	crc32_init();
//	bl_uart_write_string("initial success\r\n");
//	bl_uart_printf("lalalalalla");
	test();
	bootlader_main();
	

	
	return 0;
}
