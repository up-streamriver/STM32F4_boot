#include "main.h"



static bool check_button(void)
{	
	uint32_t start = bl_now();
	while( bl_now()-start <5000)
	{	
		if(key_is_pressed())
		{
			bl_delay_ms(100);
			return key_is_pressed();
		}
	}
	return false;

}

static void wait_release(void)
{
	while(key_is_pressed())
	{
		bl_delay_ms(100);
	}
}

extern bool bootlader_main(uint32_t boot_delay);
extern bool verify_application(void);

int main(void)
{
	 extern JUMP_APP(uint32_t base);
	//JUMP_APP(0x8010000);
	board_lowlevel_init();

	bl_uart_init();
	bl_delay_init();
	led_init();
	button_init();

	bool boot_trap = false;
	
	bl_uart_printf("5 seconds for you to press boot key\r\n");
	if(check_button())
	{
		bl_uart_printf("key pressed trap into boot\r\n");
		boot_trap = true;
	}

	if(!verify_application())
	{
		boot_trap = true;
		bl_uart_printf("app verify failed trap into boot\r\n");
	}

	if(boot_trap)
	{	
		led_set(true);
		wait_release();
	}


	bootlader_main(boot_trap? 0:3);
	

	
	return 0;
}
