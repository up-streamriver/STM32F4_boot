#include "norflash.h"

typedef struct 
{
   uint32_t sector;
   uint32_t size;
}bl_flash_desc_t;

static bl_flash_desc_t bl_flash_desc[]=
{   
    {FLASH_Sector_0,16*1024},
    {FLASH_Sector_1,16*1024},
    {FLASH_Sector_2,16*1024},
    {FLASH_Sector_3,16*1024},
    {FLASH_Sector_4,64*1024},
    {FLASH_Sector_5,128*1024},
    {FLASH_Sector_6,128*1024},
    {FLASH_Sector_7,128*1024},
    {FLASH_Sector_8,128*1024},
    {FLASH_Sector_9,128*1024},
    {FLASH_Sector_10,128*1024},
    {FLASH_Sector_11,128*1024},   
};

void bl_flash_lock(void)
{
    FLASH_Lock();
}

void bl_flash_unlock(void)
{
    FLASH_Unlock();
}

void bl_flash_erase(uint32_t address,uint32_t size)
{
    uint32_t target_address = address + size;
    uint32_t cur_address = FLASH_BASE_ADD;
    for(uint8_t i=0; i<sizeof(bl_flash_desc)/sizeof(bl_flash_desc_t);i++)
    {
        uint32_t cur_end = cur_address + bl_flash_desc[i].size;
        if(address < cur_end && target_address > cur_address)
        {
            while(FLASH_GetFlagStatus(FLASH_FLAG_BSY) == SET);
            if(FLASH_EraseSector(bl_flash_desc[i].sector,VoltageRange_3) != FLASH_COMPLETE)
            {
                bl_uart_printf("erase failed unknown\r\n");
            }
            else
            {
                bl_uart_printf("erase sector :%d size: %08x\r\n",i,bl_flash_desc[i].size);
            }
        }
        if(cur_address >= target_address)
        {
            break;
        }
        cur_address = cur_end;
    }
}

void bl_flash_write_word(uint32_t address,uint8_t *data,uint32_t size)
{
    for(uint32_t i=0;i<size;i+=4)
    {
        if(FLASH_ProgramWord(address + i,*(uint32_t *)(data+i)) != FLASH_COMPLETE)
        {
            bl_uart_printf("write failed \r\n");
        }
        else
        {
            bl_uart_printf("write success");
        }
    }
}