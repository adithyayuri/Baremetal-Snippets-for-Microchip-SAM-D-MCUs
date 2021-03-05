/*
 * Author: Adithya Yuri
 *
 * Baremetal code to configure L21 OPAMP peripheral
 */

/* NON ASF EXAMPLE FOR OPAMP PERIPHERAL IN L21
 * PUTS OPAMP IN NON-INVERTING CONFIG
 * R1 and R2 values are 8 ohms
 * GAIN IN NON-INVERTING CONFIG IS (R2/R1+1) so (8/8 + 1), GAIN = 2
 * With BIAS level 0 the bandwidth results -3db gain point - ~ 5 kHz
 * With BIAS level 3 the bandwidth results -3db gain point - ~ 400 KHz
 */


#include "sam.h"

void config_opamp(void);
void portmux(void);
void enable_opamp(void);
void enable_opamp_peripheral(void);

void enable_opamp_peripheral()
{
	// Enables OPAMP peripheral -- It is enabled by default
	MCLK->APBDMASK.bit.OPAMP_ = 1;
}

void config_opamp()
{
	// '+' Mux to POSITIVE IO pin
	OPAMP->OPAMPCTRL[0].bit.MUXPOS	= 0x0;
	// '-' Mux to Resistor ladder 0 tap
	OPAMP->OPAMPCTRL[0].bit.MUXNEG	= 0x1;
	// Enable RES1EN switch
	OPAMP->OPAMPCTRL[0].bit.RES1EN	= 0x1;
	// Enable RES2OUT switch
	OPAMP->OPAMPCTRL[0].bit.RES2OUT = 0x1;
	// Select R1=8ohms and R2=8ohms
	OPAMP->OPAMPCTRL[0].bit.POTMUX	= 0x2;
	// Connect one of the resistor ends to GND
	OPAMP->OPAMPCTRL[0].bit.RES1MUX	= 0x3;
	// Set bias to 0x3, meaning max performance and max current consumption
	OPAMP->OPAMPCTRL[0].bit.BIAS	= 0x3;
	// Enable OPAMP0
	OPAMP->OPAMPCTRL[0].bit.ENABLE	= 0x1;
}

void portmux()
{
	// Pin muxing stuff
	PORT->Group[0].PINCFG[6].bit.PMUXEN = 0x1; //PA06
	PORT->Group[0].PINCFG[7].bit.PMUXEN = 0x1; //PA07
	// Still pin muxing stuff
	PORT->Group[0].PMUX[3].bit.PMUXE = 0x1;
	PORT->Group[0].PMUX[3].bit.PMUXO = 0x1;
}

void enable_opamp()
{
	// Enable the opamp peripheral
	OPAMP->CTRLA.bit.ENABLE = 0x1;
	// Wait for OPAMP to get ready
	while(!OPAMP->STATUS.bit.READY0);
}

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
	enable_opamp_peripheral();
	config_opamp();
	portmux();
	enable_opamp();
	// Now goes to while(1) present in reset handler ------ after the main()
}
