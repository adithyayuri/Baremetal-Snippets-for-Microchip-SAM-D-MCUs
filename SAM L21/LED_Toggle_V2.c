/*
 * Author: Adithya Yuri
 */

/* Toggle pin PB10 PRGRM */

#include "sam.h"

void blinkTheDamnLED(void);
uint16_t a;

void blinkTheDamnLED()
{
	PORT->Group[1].DIR.reg = 1<<10;
	while (1)
	{
		PORT->Group[1].OUTTGL.reg = 1<<10;
		for (a=0; a<30000;a++);
	}
}

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();

	/* Setting system to use OSC16M clock */
	OSCCTRL->OSC16MCTRL.bit.FSEL = 0x3;

	blinkTheDamnLED();
}

/* Basic pin toggle L21 example */