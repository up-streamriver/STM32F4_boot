#include "main.h"
#include "flash_layout.h"
#include "arginfo.h"

void test(void)
{
	uint8_t head = 0xAA;
	uint8_t op = 0x22;
	uint16_t length = 0x0C;
	uint8_t data[12] = {0x00, 0x00, 0x01, 0x08, 0x04, 0x00, 0x00, 0x00,0x00,0x01,0x02,0x03};
	uint32_t crc = 0;
	uint32_t size = 0;
	//crc = crc32_update(crc,(uint8_t*)FLASH_APP_ADD,FLASH_APP_SIZE);
	//crc = crc32_update(crc,(uint8_t*)&op,1);
	//crc = crc32_update(crc,(uint8_t*)&length,2);
	//crc = crc32_update(crc,(uint8_t*)&data,length);
	//  bl_flash_unlock();
	//  bl_flash_erase(FLASH_ARG_ADD,16*1024);
	//  bl_flash_lock();
	//  bl_flash_unlock();
	//  bl_flash_write_word(FLASH_ARG_ADD,&arg,sizeof(arg));
	//  bl_flash_lock(); 
	//  bl_flash_unlock();
	//  bl_flash_write_word(FLASH_ARG_ADD+4,&size,sizeof(size));
	//  bl_flash_lock(); 	
	//  bl_flash_unlock();
	//  bl_flash_write_word(FLASH_ARG_ADD+8,&crc,sizeof(crc));
	//  bl_flash_lock(); 	
	arginfo_read(&size,&crc);
	bl_uart_printf("size : %08x",size);
	bl_uart_printf("crc : %08x",crc);
}

int main(void)
{
	 extern JUMP_APP(uint32_t base);
	//JUMP_APP(0x8010000);
	
	board_lowlevel_init();
	bl_uart_init();
	crc32_init();
//	bl_uart_write_string("initial success\r\n");
//	bl_uart_printf("lalalalalla");
	test();
	
	bootlader_main();
	

	
	return 0;
}
