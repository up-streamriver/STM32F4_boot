#include "arginfo.h"
#define LOG_LVL     ELOG_LVL_INFO
#define LOG_TAG     "arginfo"
#include "elog.h"
#define FLASH_ARG_MAGIC 0x12345678
/*
arg 0x12345678
crc A2DA1004
size0000266C
*/

bool arginfo_read(uint32_t *size,uint32_t *crc)
{
    uint32_t* arginfo = (uint32_t *)(FLASH_ARG_ADD);
    //log_i("magic : %08x",arginfo[0]);
    if(arginfo[0] != FLASH_ARG_MAGIC)
    {   
        return false;
    }
    if(size)
    {		
        *size = arginfo[1];
    }
    
    if(crc)
    {
        *crc = arginfo[2];
    }
    return true;

}
