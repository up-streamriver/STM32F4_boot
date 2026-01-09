#include "button.h"
/*
key0 pe0
*/

#define KEY_GPIO_PORT GPIOE
#define KEY_GPIO_PIN GPIO_Pin_0
#define KEY_GPIO_PIN_1 GPIO_Pin_1

void button_init(void)
{
    GPIO_InitTypeDef GPIO_Structure;
    GPIO_Structure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_Structure.GPIO_Pin = KEY_GPIO_PIN | KEY_GPIO_PIN_1;
    GPIO_Structure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Structure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIO_PORT,&GPIO_Structure);
}

bool button_is_pressed(void)
{
    return GPIO_ReadInputDataBit(KEY_GPIO_PORT,KEY_GPIO_PIN) == Bit_RESET;
}

bool key_is_pressed(void)
{
    return GPIO_ReadInputDataBit(KEY_GPIO_PORT,KEY_GPIO_PIN_1) == Bit_RESET;
}