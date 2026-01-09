#include "led.h"
/*
pc13 LED2
*/

#define LED_GPIO_PORT GPIOC
#define LED_GPIO_PIN GPIO_Pin_13

void led_init(void)
{
    GPIO_InitTypeDef GPIOStructure;
    GPIOStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIOStructure.GPIO_OType = GPIO_OType_PP;
    GPIOStructure.GPIO_Pin = LED_GPIO_PIN;
    GPIOStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIOStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO_PORT,&GPIOStructure);

    led_set(false);
}

void led_set(bool status)
{
    if(status)
    {
        GPIO_WriteBit(LED_GPIO_PORT,LED_GPIO_PIN,Bit_RESET);
    }
    else
    {
        GPIO_WriteBit(LED_GPIO_PORT,LED_GPIO_PIN,Bit_SET);
    }
}

