#include "stm32f4xx.h"
#include <stdio.h>
#include "flash_layout.h"

void boot_application(void)
{
    typedef int (*entry_t)(void);

    uint32_t address = FLASH_APP_ADD;
    uint32_t _sp = *(uint32_t *)(address + 0);
    uint32_t _pc = *(uint32_t *)(address + 4);
    (void)_sp;
    entry_t entry = (entry_t)_pc;

    bl_uart_printf("booting application at 0x%08X",address);

    bl_lowlevel_deinit();

    __set_MSP(_sp); 
    entry();
}