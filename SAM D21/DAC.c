/*
 * Author: Adithya Yuri
 *
 * Baremetal DAC example snippet
 *
 */



#include <asf.h>

int main (void)
{
	system_init();
	PM->APBCMASK.bit.DAC_ = 1;

	PORT->Group[0].PINCFG[2].bit.PMUXEN = 1;
	PORT->Group[0].PMUX[1].bit.PMUXE = 0x01;

	GCLK->CLKCTRL.reg=GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK4 | GCLK_CLKCTRL_ID_DAC;

	DAC->CTRLB.reg = DAC_CTRLB_REFSEL_INT1V | DAC_CTRLB_EOEN;

	DAC->CTRLA.reg = DAC_CTRLA_ENABLE;
	while(DAC->STATUS.bit.SYNCBUSY);

	DAC->DATA.reg = 0x3FF;
	while(DAC->STATUS.bit.SYNCBUSY);

	while (1)
	{
		for (uint16_t a=0x00;a<0x3ff;a=a+0xF)
		{
			DAC->DATABUF.reg = a;
			while(DAC->STATUS.bit.SYNCBUSY);
		}
	}
}
