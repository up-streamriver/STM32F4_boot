#include "stm32f4xx.h"
#include <stdio.h>
#include <stdbool.h>
#include "flash_layout.h"
#include "bl_uart.h"
#include "ringbuffer.h"
#include "crc32.h"

#define bl_uart_buffer_size 512
#define BL_TIME_OUT_MS   512
#define bl_pkt_head_size    (1+1+2)
#define bl_pkt_base_size    (1+1+2+4)
#define bl_pkt_payload_size    4096
#define bl_pkt_total_size    bl_pkt_payload_size + bl_pkt_base_size

#define BL_INQUIRY_VERSION_MAJOR 1
#define BL_INQUIRY_VERSION_MINOR 0

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
    BL_MTU_SIZE,
}bl_inquiry_code_t;

typedef struct 
{
    bl_op_t opcode;
    uint16_t length;
    uint32_t crc;
    uint8_t data[bl_pkt_payload_size];
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
}bl_inquiry_t;

typedef struct 
{
    uint32_t address;
    uint32_t size;
}bl_erase_t;

typedef struct 
{
    uint32_t address;
    uint32_t size;
    uint8_t data[];
}bl_write_t;


static uint8_t bl_uart_buffer[bl_uart_buffer_size];
static ringbuffer_t serial_rx;
static last_pkt_time;
bl_ctrl_t bl_ctrl;

static void bl_uart_recv_temp_store(uint8_t data)
{
	rb8_puts(serial_rx,&data,1);
}

static bl_response(bl_op_t op,uint8_t *data,uint16_t length)
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

static bl_response_ack(bl_op_t op,bl_err_t err)
{
    bl_response(op,(uint8_t*)&err,1);
}

static bl_reset_sm(bl_ctrl_t *bl_ctrl_t)
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
    bl_inquiry_t *bl_inquiry = (bl_inquiry_t*)data;
    bl_uart_printf("data:%08x\r\n",bl_inquiry->subcode);
    if(sizeof(bl_inquiry_t) != length)
    {   
        bl_uart_printf("inquiry length error\r\n");
        //bl_response_ack(BL_OP_INQUIRY,BL_ERR_PARAM);
    }
    switch (bl_inquiry->subcode)
    {
    case BL_INQUIRY_VERSION:
    {
        uint8_t version[] = {BL_INQUIRY_VERSION_MAJOR,BL_INQUIRY_VERSION_MINOR}; 
        bl_uart_write_data((uint8_t *)version,sizeof(version));
        bl_uart_printf("inquiry version success\r\n");
        //bl_response(BL_OP_INQUIRY,(uint8_t *)version,sizeof(version));
        break;
    }
    case BL_MTU_SIZE:
    {
        uint8_t mut_size = bl_pkt_payload_size; 
        bl_uart_write_data((uint8_t *)mut_size,sizeof(mut_size));
        bl_uart_printf("inquiry mtu success\r\n");
        //bl_response(BL_OP_INQUIRY,(uint8_t *)mut_size,sizeof(mut_size));
        break;
    }
    default:
    {   
        bl_uart_printf("inquiry subcode error\r\n");
        //bl_response_ack(BL_OP_INQUIRY,BL_ERR_PARAM);
        break;
    }
    }

}

static void bl_op_boot_handler(uint8_t *data,uint16_t length)
{   
    bl_uart_printf("boot success\r\n");
    boot_application();
}

static void bl_op_reset_handler(uint8_t *data,uint16_t length)
{   
    bl_uart_printf("reset waiting\r\n");
    NVIC_SystemReset();
}

static void bl_op_erase_handler(uint8_t *data,uint16_t length)
{
    bl_erase_t *bl_erase = (bl_erase_t*)data;
    bl_uart_printf("erase add:%08x\r\n",bl_erase->address);
    bl_uart_printf("erase size:%08x\r\n",bl_erase->size);
    if(sizeof(bl_erase_t) != length)
    {   
        bl_uart_printf("erase length error\r\n");
        //bl_response_ack(BL_OP_ERASE,BL_ERR_PARAM);
    }
    bl_flash_unlock();
	bl_flash_erase(bl_erase->address,bl_erase->size);
	bl_flash_lock();
    bl_uart_printf("erase success");
}

static void bl_op_read_handler(uint8_t *data,uint16_t length)
{
    ;
}

static void bl_op_write_handler(uint8_t *data,uint16_t length)
{
    bl_write_t *bl_write = (bl_write_t*)data;
    bl_uart_printf("write add:%08x\r\n",bl_write->address);
    bl_uart_printf("write size:%08x\r\n",bl_write->size);
    if(sizeof(bl_write_t) +bl_write->size != length)
    {   
        bl_uart_printf("write length error\r\n");
        //bl_response_ack(BL_OP_WRITE,BL_ERR_PARAM);
    }
    bl_flash_unlock();
	bl_flash_write_word(bl_write->address,bl_write->data,bl_write->size);
	bl_flash_lock();    
}

static void bl_op_verify_handler(uint8_t *data,uint16_t length)
{
    
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
            bl_uart_printf("sm idle\r\n");
            rx->index = 0;
            if(rx->data[0] == 0xAA)
            {
                bl_ctrl->sm = BL_SM_START;
                bl_uart_printf("head = %02x\r\n",rx->data[0]);
            }
            break;
        }
        case BL_SM_START:
        {
            bl_uart_printf("sm start\r\n");
            rx->index = 0;
            pkt->opcode = (bl_op_t)rx->data[0];
            bl_ctrl->sm = BL_SM_OPCODE;
            bl_uart_printf("op = %02x\r\n",rx->data[0]);
            break;
        }
        case BL_SM_OPCODE:
        {   
            bl_uart_printf("sm opcode\r\n");
            if(rx->index == 2)
            {
                rx->index = 0;
                uint16_t length = *(uint16_t *)rx->data;
                if(length <= bl_pkt_payload_size)
                {
                    pkt->length = length;
                    bl_uart_printf("length = %04x\r\n",length);
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
            bl_uart_printf("sm param\r\n");
            rx->index = 0;
            if(pkt->index < pkt->length)
            {
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
            bl_uart_printf("sm crc\r\n");
            if(rx->index == 4)
            {
                rx->index = 0;
                uint32_t crc = *(uint32_t *)rx->data;
                if(bl_pkt_verify(pkt,crc))
                {
                    pkt->crc = crc;
                    //last_pkt_time = bl_now();
                    full_pkt = true;
                    bl_uart_printf("crc ok\r\n");
                    bl_reset_sm(bl_ctrl);
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

void bootlader_main(void)
{
    serial_rx = rb8_new(bl_uart_buffer,bl_uart_buffer_size);
	bl_uart_recv_callback_register(bl_uart_recv_temp_store);
    bl_reset_sm(&bl_ctrl);
    bl_uart_printf(": sm = %d (BL_SM_IDLE=%d)\r\n", bl_ctrl.sm, BL_SM_IDLE);

    while(1)
    {   
        if (rb8_is_empty(serial_rx))
            continue;
        uint8_t data;
        if(rb8_gets(serial_rx,&data,1))
        {
            // bl_uart_printf("%02x\r\n",data);
            if(bl_uart_recv_handler(&bl_ctrl,data))
                bl_fullpkt_handler(&bl_ctrl.pkt);
        }
        
    }

}



void boot_application(void);


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