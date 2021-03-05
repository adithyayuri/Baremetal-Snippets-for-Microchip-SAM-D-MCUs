/*
 * Author: Adithya Yuri
 */

/* Toggle pin PB10 PRGRM */

#include "sam.h"

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();

	/* Setting system to use OSC16M clock */
	OSCCTRL->OSC16MCTRL.bit.FSEL = 0x3;

	PORT->Group[1].DIR.reg = 1<<10;

	while (1)
	{
		PORT->Group[1].OUTTGL.reg = 1<<10;
	}
}

/* Basic pin toggle L21 example */