/*
USART1
RX: PA10
TX: PA9
APB2/84MHz

USART2
RX: PA3
TX: PA2
APB1/42MHz
*/
#include "bl_uart.h"
bl_uart_recv_callback_t bl_uart_recv_callback;
void bl_uart_init(void)
{
    GPIO_InitTypeDef GPIOInitStructure;
    USART_InitTypeDef USARTInitStructure;
    NVIC_InitTypeDef NVICInitStructure;

    GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2);

    GPIOInitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIOInitStructure.GPIO_OType = GPIO_OType_PP;
    GPIOInitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIOInitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIOInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIOInitStructure);

    USARTInitStructure.USART_BaudRate = 115200;
    USARTInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USARTInitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USARTInitStructure.USART_Parity = USART_Parity_No;
    USARTInitStructure.USART_StopBits = USART_StopBits_1;
    USARTInitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2,&USARTInitStructure);

    NVICInitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVICInitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVICInitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVICInitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVICInitStructure);

    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART2,ENABLE);



    GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);

    GPIOInitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIOInitStructure.GPIO_OType = GPIO_OType_PP;
    GPIOInitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIOInitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIOInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIOInitStructure);

    USARTInitStructure.USART_BaudRate = 115200;
    USARTInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USARTInitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USARTInitStructure.USART_Parity = USART_Parity_No;
    USARTInitStructure.USART_StopBits = USART_StopBits_1;
    USARTInitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1,&USARTInitStructure);

    NVICInitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVICInitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVICInitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVICInitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVICInitStructure);

    USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART1,ENABLE);    

}

void bl_uart_write_data(uint8_t *data,uint32_t length)
{
    for(uint32_t i=0;i<length;i++)
    {
        while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET);
        USART_SendData(USART2,data[i]);
    }
    while(USART_GetFlagStatus(USART2,USART_FLAG_TC) == RESET);
}

void bl_uart_write_string(const char* str)
{
    uint32_t length = strlen(str);
    bl_uart_write_data((uint8_t *)str,length);
}

void bl_uart_printf(char *format,...)
{   
    char buffer[128];
    va_list vags;
    va_start(vags,format);
    vsnprintf(buffer,sizeof(buffer),format,vags);
    va_end(vags);

    bl_uart_write_string(buffer);
}

void bl_uart_recv_callback_register(bl_uart_recv_callback_t callback)
{
    bl_uart_recv_callback = callback;
}

void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2,USART_IT_RXNE) == SET)
    {
        uint8_t data = USART_ReceiveData(USART2);
        bl_uart_write_data(&data,1);
    }
}

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1,USART_IT_RXNE) == SET)
    {
        uint8_t data = USART_ReceiveData(USART1);
        if(bl_uart_recv_callback)
        {
            bl_uart_recv_callback(data);
        }
    }
}