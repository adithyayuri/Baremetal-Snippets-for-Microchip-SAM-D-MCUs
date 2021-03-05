/*
 * Author: Adithya Yuri
 *
 * PB30 is configured as output and indefnitely toggled under while(1)
 */

#include "sam.h"

int main(void)
{
	PORT->Group[1].DIRSET.reg = (1u << 30);
    while (1)
    {
        PORT->Group[1].OUTTGL.reg = (1u << 30);
		for (volatile uint16_t i = 0; i < 10000; i++);
    }
}
