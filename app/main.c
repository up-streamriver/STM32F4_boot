#include <stdint.h>
#include "stm32f4xx.h"

int main(void)
{
	extern JUMP_APP(uint32_t base);
	JUMP_APP(0x8010000);
	return 0;
}
