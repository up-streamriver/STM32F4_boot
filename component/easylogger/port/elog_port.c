/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */
 
#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "main.h"
#include "elog.h"

/**
 * EasyLogger port initialize
 *
 * @return result
 */
 
 extern uint32_t bl_now(void);
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    GPIO_InitTypeDef GPIOStructure;
    NVIC_InitTypeDef NVICStructure;
    USART_InitTypeDef USARTStructure;

    GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2);

    GPIOStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIOStructure.GPIO_OType = GPIO_OType_PP;
    GPIOStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIOStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIOStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIOStructure);
    


    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVICStructure.NVIC_IRQChannel = USART2_IRQn;
    NVICStructure.NVIC_IRQChannelCmd = ENABLE;
    NVICStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVICStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVICStructure);

    USARTStructure.USART_BaudRate = 115200;
    USARTStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USARTStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USARTStructure.USART_Parity = USART_Parity_No;
    USARTStructure.USART_StopBits = USART_StopBits_1;
    USARTStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART2,&USARTStructure);

    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART2,ENABLE);
    
    
    
    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {
    USART_Cmd(USART2,DISABLE);
    USART_DeInit(USART2);
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    USART_ClearFlag(USART2,USART_FLAG_TC);
    for(uint32_t i=0; i<size; i++)
    {
        while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET);
        USART_SendData(USART2,(uint8_t)log[i]);
    }
    while(USART_GetFlagStatus(USART2,USART_FLAG_TC) == RESET);
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    
    /* add your code here */
    
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    
    /* add your code here */
    
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
    static char stime[32];
    sprintf(stime,"%06ul",bl_now());
    return stime;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    
    /* add your code here */
    return "";
    
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    
    /* add your code here */
    return "";
    
}