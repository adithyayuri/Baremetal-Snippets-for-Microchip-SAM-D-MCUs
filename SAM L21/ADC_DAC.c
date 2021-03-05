/*
 * Author: Adithya Yuri
 *
 * Baremetal code for ADC and DAC peripherals
 *
 * ADC Pin: PA03
 * DAC Pin: PA02
 */

#include "sam.h"

void configDAC0(void);
void pinMuxDAC(void);
void configADC(void);
void pinMuxADC(void);

void configDAC0() {
	MCLK->APBCMASK.bit.DAC_		= 0x1;

	GCLK->GENCTRL[1].reg		= GCLK_GENCTRL_SRC_OSC16M | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_IDC | GCLK_GENCTRL_DIV(2);
	GCLK->PCHCTRL[32].reg		= GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1;

	DAC->CTRLB.bit.REFSEL		= 0x1;
	DAC->DACCTRL[0].bit.CCTRL	= 0x2;
	DAC->DACCTRL[0].bit.REFRESH	=
	DAC->DACCTRL[0].bit.ENABLE	= 0x1;

	DAC->CTRLA.bit.ENABLE		= 0x1;
	while(DAC->SYNCBUSY.bit.ENABLE);

	pinMuxDAC();
	while(!DAC->INTFLAG.bit.EMPTY0);
	DAC->DATA[0].reg			= 0x3FF;
	while(DAC->SYNCBUSY.bit.DATA0);

}

void pinMuxDAC() {
	PORT->Group[0].PINCFG[2].bit.PMUXEN = 0x1; //PA02 DAC output
	PORT->Group[0].PMUX[1].bit.PMUXE	= 0x1; //B function
}

void configADC() {
	MCLK->APBDMASK.bit.ADC_		= 0x1;
	GCLK->PCHCTRL[30].reg		= GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK0;
	ADC->CTRLB.bit.PRESCALER	= 0x0;
	ADC->REFCTRL.bit.REFSEL		= ADC_REFCTRL_REFSEL_INTVCC2_Val;
//	ADC->REFCTRL.bit.REFSEL		= ADC_REFCTRL_REFSEL_INTREF_Val;
	ADC->REFCTRL.bit.REFCOMP	= 0x1;

//	SUPC->VREF.reg |= SUPC_VREF_TSEN;

//	ADC->INPUTCTRL.bit.MUXNEG	= 0x18;
	ADC->INPUTCTRL.bit.MUXPOS	= 0x1;;
	while(ADC->SYNCBUSY.bit.INPUTCTRL);

	ADC->CTRLC.bit.RESSEL		= ADC_CTRLC_RESSEL_12BIT_Val;
	while(ADC->SYNCBUSY.bit.CTRLC);
	ADC->CTRLC.bit.FREERUN		= 0x1;
	while(ADC->SYNCBUSY.bit.CTRLC);

	ADC->CTRLA.bit.ENABLE		= 0x1;
	while(ADC->SYNCBUSY.bit.ENABLE);

	ADC->SWTRIG.bit.START		= 0x1;
	while(ADC->SYNCBUSY.bit.SWTRIG);
}

pinMuxADC() {
	PORT->Group[0].PINCFG[3].bit.PMUXEN = 0x1; //PA03 ADC input
	PORT->Group[0].PMUX[1].bit.PMUXE	= 0x1; //B function
}

int main(void) {

	SystemInit();
    // Switch to 16M oscillator
	OSCCTRL->OSC16MCTRL.bit.FSEL = 0x3;

	configDAC0();
	configADC();

	while(1) {
		for (uint16_t a=0; a<0xFFF; (a=a+0x1)) {
			while(!DAC->INTFLAG.bit.EMPTY0);
			DAC->DATA[0].reg	= a;
			while(DAC->SYNCBUSY.bit.DATA0);
		}
	}
}
