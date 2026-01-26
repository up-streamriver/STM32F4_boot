#include "stm32f4xx.h"
#include <stdio.h>
#include <stdbool.h>
#include "flash_layout.h"
#include "bl_uart.h"
#include "ringbuffer.h"
#include "crc32.h"
#include "arginfo.h"
#include "board.h"


#define bl_uart_buffer_size 512
#define BL_TIME_OUT_MS   512
#define bl_pkt_head_size    (1+1+2)
#define bl_pkt_base_size    8ul
#define bl_pkt_payload_size    256ul
#define bl_pkt_total_size    (bl_pkt_payload_size + bl_pkt_base_size)

#define BL_INQUIRY_VERSION_MAJOR 1
#define BL_INQUIRY_VERSION_MINOR 0

#define LOG_LVL     ELOG_LVL_INFO
#define LOG_TAG     "boot"
#include "elog.h"

typedef enum
{
    BL_SM_IDLE,
    BL_SM_START,
    BL_SM_OPCODE,
    BL_SM_LENGTH,
    BL_SM_PARAM,
    BL_SM_CRC,
}bl_machine_state_t;

typedef enum
{
    BL_OP_NONE = 0x00,
    BL_OP_INQUIRY = 0x10,
    BL_OP_BOOT = 0x11,
    BL_OP_RESET = 0x1F,
    BL_OP_ERASE = 0x20,
    BL_OP_READ,
    BL_OP_WRITE,
    BL_OP_VERIFY,
}bl_op_t;

typedef enum
{
    BL_ERR_OK,
    BL_ERR_OPCODE,
    BL_ERR_OVERFLOW,
    BL_ERR_TIMEOUT,
    BL_ERR_FORMAT,
    BL_ERR_VERIFY,
    BL_ERR_PARAM,
    BL_ERR_UNKNOWN = 0xFF,
}bl_err_t;

typedef enum
{
    BL_INQUIRY_VERSION,
    BL_INQUIRY_MTU_SIZE,
}bl_inquiry_t;

typedef struct 
{
    bl_op_t opcode;
    uint16_t length;
    uint32_t crc;
    uint8_t data[bl_pkt_total_size];
    uint16_t index;
}bl_pkt_t;

typedef struct 
{
    uint8_t data[16];
    uint16_t index;
}bl_rx_t;

typedef struct 
{
    bl_pkt_t pkt;
    bl_rx_t rx;
    bl_machine_state_t sm;
}bl_ctrl_t;


typedef struct 
{
    uint8_t subcode;
}bl_inquiry_param_t;

typedef struct 
{
    uint32_t address;
    uint32_t size;
}bl_erase_param_t;

typedef struct 
{
    uint32_t address;
    uint32_t size;
    uint8_t data[];
}bl_write_param_t;

typedef struct 
{   
    uint32_t address;
    uint32_t size;
    uint32_t crc;
}bl_verify_param_t;


static uint8_t bl_uart_buffer[bl_uart_buffer_size];
static ringbuffer_t serial_rx;
static uint32_t  last_pkt_time = 0;
bl_ctrl_t bl_ctrl;

void boot_application(void);

static void bl_uart_recv_temp_store(uint8_t *data, uint32_t len)
{
    rb8_puts(serial_rx, data, len);
}

static void bl_response(bl_op_t op,uint8_t *data,uint16_t length)
{   
    uint8_t head = 0xAA;
    uint32_t crc = 0;
    crc = crc32_update(crc,(uint8_t *)&head,1);
    crc = crc32_update(crc,(uint8_t *)&op,1);
    crc = crc32_update(crc,(uint8_t *)&length,2);
    crc = crc32_update(crc,data,length);

    bl_uart_write_data((uint8_t *)&head,1);
    bl_uart_write_data((uint8_t *)&op,1);
    bl_uart_write_data((uint8_t *)&length,2);
    bl_uart_write_data(data,length);
    bl_uart_write_data((uint8_t *)&crc,4);

}

static void bl_response_ack(bl_op_t op,bl_err_t err)
{
    bl_response(op,(uint8_t*)&err,1);
}

static void bl_reset_sm(bl_ctrl_t *bl_ctrl_t)
{
    bl_ctrl_t->pkt.index = 0;
    bl_ctrl_t->rx.index = 0;
    bl_ctrl_t->sm = BL_SM_IDLE;
}

bool bl_pkt_verify(bl_pkt_t *pkt,uint32_t ccrc)
{
    uint8_t head = 0xAA;
    uint32_t crc = 0;
    crc = crc32_update(crc,(uint8_t *)&head,1);
    crc = crc32_update(crc,(uint8_t *)&pkt->opcode,1);
    crc = crc32_update(crc,(uint8_t *)&pkt->length,2);
    crc = crc32_update(crc,(uint8_t *)&pkt->data,pkt->length);

    return crc == ccrc;
}

static void bl_op_inquiry_handler(uint8_t *data,uint16_t length)
{
    bl_inquiry_param_t *bl_inquiry = (bl_inquiry_param_t*)data;
    log_i("data:%08x\r\n",bl_inquiry->subcode);
    if(sizeof(bl_inquiry_param_t) != length)
    {   
        log_i("inquiry length error\r\n");
        //bl_response_ack(BL_OP_INQUIRY,BL_ERR_PARAM);
    }
    switch (bl_inquiry->subcode)
    {
    case BL_INQUIRY_VERSION:
    {   
        led_set(false);
        uint8_t version[] = {BL_INQUIRY_VERSION_MAJOR,BL_INQUIRY_VERSION_MINOR}; 
        log_i("inquiry version success\r\n");
        bl_response(BL_OP_INQUIRY,(uint8_t *)version,sizeof(version));
        break;
    }
    case BL_INQUIRY_MTU_SIZE:
    {
        uint16_t mut_size = bl_pkt_payload_size; 
        log_i("inquiry mtu success\r\n");
        bl_response(BL_OP_INQUIRY,(uint8_t *)&mut_size,sizeof(mut_size));
        break;
    }
    default:
    {   
        log_i("inquiry subcode error\r\n");
        bl_response_ack(BL_OP_INQUIRY,BL_ERR_PARAM);
        break;
    }
    }

}

static void bl_op_boot_handler(uint8_t *data,uint16_t length)
{   
    log_i("boot success\r\n");
    boot_application();
}

static void bl_op_reset_handler(uint8_t *data,uint16_t length)
{   
    log_i("reset waiting\r\n");
    NVIC_SystemReset();
}

static void bl_op_erase_handler(uint8_t *data,uint16_t length)
{
    bl_erase_param_t *bl_erase = (bl_erase_param_t*)data;
    log_i("erase add:%08x\r\n",bl_erase->address);
    log_i("erase size:%08x\r\n",bl_erase->size);
    if(sizeof(bl_erase_param_t) != length)
    {   
        log_i("erase length error\r\n");
        //bl_response_ack(BL_OP_ERASE,BL_ERR_PARAM);
    }
    bl_flash_unlock();
	bl_flash_erase(bl_erase->address,bl_erase->size);
	bl_flash_lock();
    log_i("erase success");
    bl_response_ack(BL_OP_ERASE, BL_ERR_OK);
}

static void bl_op_read_handler(uint8_t *data,uint16_t length)
{
    ;
}

static void bl_op_write_handler(uint8_t *data,uint16_t length)
{
    bl_write_param_t *bl_write = (bl_write_param_t*)data;
    log_i("write add:%08x\r\n",bl_write->address);
    log_i("write size:%08x\r\n",bl_write->size);
    if(sizeof(bl_write_param_t) +bl_write->size != length)
    {   
        log_i("write length error\r\n");
        //bl_response_ack(BL_OP_WRITE,BL_ERR_PARAM);
    }
    bl_flash_unlock();
	bl_flash_write_word(bl_write->address,bl_write->data,bl_write->size);
	bl_flash_lock();  
    log_i("write success");
    bl_response_ack(BL_OP_WRITE, BL_ERR_OK);  
}

static bool bl_op_verify_handler(uint8_t *data,uint16_t length)
{
    bl_verify_param_t *bl_verify = (bl_verify_param_t*)data;
    log_i("address:%08x\r\n",bl_verify->address);
    log_i("size:%08x\r\n",bl_verify->size);
    log_i("crc:%08x\r\n",bl_verify->crc);
    if(sizeof(bl_verify_param_t) != length)
    {   
        log_i("verify length error\r\n");
        //bl_response_ack(BL_OP_VERIFY,BL_ERR_PARAM);
    }
    uint32_t crc = 0;
    crc = crc32_update(crc,(uint8_t *)bl_verify->address,bl_verify->size);
    return crc == bl_verify->crc;
}

static void bl_fullpkt_handler(bl_pkt_t *pkt)
{
    switch(pkt->opcode)
    {
        case BL_OP_INQUIRY:
        {
            bl_op_inquiry_handler(pkt->data,pkt->length);
            break;
        }
        case BL_OP_BOOT:
        {
            bl_op_boot_handler(pkt->data,pkt->length);
            break;
        }
        case BL_OP_RESET:
        {
            bl_op_reset_handler(pkt->data,pkt->length);
            break;
        }    
        case BL_OP_ERASE:
        {
            bl_op_erase_handler(pkt->data,pkt->length);
            break;
        }
        case BL_OP_READ:
        {
            bl_op_read_handler(pkt->data,pkt->length);
            break;
        }
        case BL_OP_WRITE:
        {
            bl_op_write_handler(pkt->data,pkt->length);
            break;
        }
        case BL_OP_VERIFY:
        {
            bl_op_verify_handler(pkt->data,pkt->length);
            break;
        }                                           
    }
}

bool bl_uart_recv_handler(bl_ctrl_t *bl_ctrl,uint8_t data)
{   
    bool full_pkt = false;
    bl_pkt_t *pkt = &bl_ctrl->pkt;
    bl_rx_t *rx = &bl_ctrl->rx;
    rx->data[rx->index ++] = data;
    switch(bl_ctrl->sm)
    {
        case BL_SM_IDLE:
        {   
            log_i("sm idle\r\n");
            rx->index = 0;
            if(rx->data[0] == 0xAA)
            {
                bl_ctrl->sm = BL_SM_START;
                log_i("head = %02x\r\n",rx->data[0]);
            }
            break;
        }
        case BL_SM_START:
        {
            log_i("sm start\r\n");
            rx->index = 0;
            pkt->opcode = (bl_op_t)rx->data[0];
            bl_ctrl->sm = BL_SM_OPCODE;
            log_i("op = %02x\r\n",rx->data[0]);
            break;
        }
        case BL_SM_OPCODE:
        {   
            log_i("sm opcode\r\n");
            if(rx->index == 2)
            {
                rx->index = 0;
                uint16_t length = *(uint16_t *)rx->data;
                if(length <= bl_pkt_total_size)
                {
                    pkt->length = length;
                    log_i("length = %04x\r\n",length);
                    if(length == 0)   bl_ctrl->sm = BL_SM_CRC;
                    else              bl_ctrl->sm = BL_SM_PARAM;
                }
                else
                {   
                    bl_response_ack(pkt->opcode,BL_ERR_OVERFLOW);
                    bl_reset_sm(bl_ctrl);
                }
            }
            break;
        }
        case BL_SM_PARAM:
        {   
            log_i("sm param\r\n");
            rx->index = 0;
            if(pkt->index < pkt->length)
            {   
                log_i("pkt->index :%d\r\n",pkt->index);
                pkt->data[pkt->index++] = rx->data[0];
                if(pkt->index == pkt->length)
                {
                    bl_ctrl->sm = BL_SM_CRC;
                }
            }
            break;
        }
        case BL_SM_CRC:
        {   
            log_i("sm crc\r\n");
            if(rx->index == 4)
            {
                rx->index = 0;
                uint32_t crc = *(uint32_t *)rx->data;
                if(bl_pkt_verify(pkt,crc))
                {
                    pkt->crc = crc;
                    //last_pkt_time = bl_now();
                    full_pkt = true;
                    log_i("crc ok\r\n");                   
                  //  bl_response_ack(pkt->opcode,BL_ERR_OK);
                }
                else
                {
                    bl_reset_sm(bl_ctrl);
                    bl_response_ack(pkt->opcode,BL_ERR_VERIFY);                   
                }

            }
            break;
        }
        default:
            {
                bl_reset_sm(bl_ctrl);
                bl_response_ack(pkt->opcode,BL_ERR_UNKNOWN);
                break;                     
            }
            
    }
    return full_pkt;
    

}

// void bootlader_main(void)
// {
//     serial_rx = rb8_new(bl_uart_buffer,bl_uart_buffer_size);
// 	bl_uart_recv_callback_register(bl_uart_recv_temp_store);
//     bl_reset_sm(&bl_ctrl);
//     log_i(": sm = %d (BL_SM_IDLE=%d)\r\n", bl_ctrl.sm, BL_SM_IDLE);

//     while(1)
//     {   
//         if (rb8_is_empty(serial_rx))
//             continue;
//         uint8_t data;
//         if(rb8_gets(serial_rx,&data,1))
//         {
//             // log_i("%02x\r\n",data);
//             if(bl_uart_recv_handler(&bl_ctrl,data))
//                 bl_fullpkt_handler(&bl_ctrl.pkt);
//         }
        
//     }

// }

void bootlader_main(uint32_t boot_delay)
{   
    bool boot_trap = false;
    static uint32_t main_enter_time = 0;
    bl_uart_recv_callback_register(bl_uart_recv_temp_store);
    serial_rx = rb8_new(bl_uart_buffer,bl_uart_buffer_size);
    main_enter_time = bl_now();
    while(1)
    {
        if(boot_delay > 0 && !boot_trap)
        {   
            static uint32_t last_passed_time = 0;
            uint32_t time_passed = bl_now() - main_enter_time;
            if(last_passed_time == 0)
            {
                log_i("boot app in %d seconds\r\n",boot_delay);
                last_passed_time = 1;
                time_passed = 1;
            }
            else if(last_passed_time / 1000 != time_passed / 1000)
            {
                log_i("boot app in %d seconds\r\n",boot_delay - time_passed / 1000);
            }
            if(time_passed > boot_delay * 1000)
            {
                boot_application();
            }
            last_passed_time = time_passed;
        }
        
        if(button_is_pressed())
        {
            bl_delay_ms(10);
            if(button_is_pressed())
            {
                led_set(false);
			    log_i("waiting reset");
			    while(button_is_pressed());
			    NVIC_SystemReset();
            }
        }


        if (rb8_is_empty(serial_rx))
        {
            if(bl_ctrl.rx.index == 0)
            {   
                last_pkt_time = bl_now();
            }
            else
            {
                if(bl_now() - last_pkt_time > BL_TIME_OUT_MS)
                {
                    bl_reset_sm(&bl_ctrl);
                    log_i("recv data timeout\r\n");
                }
            }
            continue;
        }
        uint8_t data;
        if(rb8_gets(serial_rx,&data,1))
        {
            if(bl_uart_recv_handler(&bl_ctrl,data))
            {
                bl_fullpkt_handler(&bl_ctrl.pkt);
                bl_reset_sm(&bl_ctrl);
                boot_trap = true;
                last_pkt_time = bl_now();
            }
        }        
    }


}



bool verify_application(void)
{   
    uint32_t size;
    uint32_t crc;
    if(!arginfo_read(&size,&crc))
    {
        return false;
    }
    uint32_t ccrc = 0;
    ccrc = crc32_update(ccrc,(uint8_t *)FLASH_APP_ADD,size);
    log_i("size : %08x\r\n",size);
    log_i("ccrc : %08x\r\n",ccrc);
    return ccrc == crc;
    
}


void boot_application(void)
{
    typedef int (*entry_t)(void);

    uint32_t address = FLASH_APP_ADD;
    uint32_t _sp = *(uint32_t *)(address + 0);
    uint32_t _pc = *(uint32_t *)(address + 4);
    (void)_sp;
    entry_t entry = (entry_t)_pc;

    log_i("booting application at 0x%08X",address);

    bl_lowlevel_deinit();

    __set_MSP(_sp); 
    entry();
}
