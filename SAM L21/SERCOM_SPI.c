/*
 * Author: Adithya Yuri
 *
 * Simple Baremetal code for L21 SERCOM SPI
 *
 */


#include "sam.h"

void clock_init(void);
void sercom_spi_init(void);
void spi_send(uint8_t);

void clock_init()
{
	// Switch to 16M
    OSCCTRL->OSC16MCTRL.bit.FSEL = 0x3;
}

void sercom_spi_init()
{
	/* SPI CLK INIT */
	GCLK->PCHCTRL[24].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;
	MCLK->APBDMASK.bit.SERCOM5_ = 1;

	SERCOM5->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_FORM(0) | SERCOM_SPI_CTRLA_DIPO(0) | SERCOM_SPI_CTRLA_DOPO(1) | SERCOM_SPI_CTRLA_MODE(0x3) ;
	SERCOM5->SPI.BAUD.reg = 0x1;

	while(SERCOM5->SPI.SYNCBUSY.bit.CTRLB);
	SERCOM5->SPI.CTRLA.bit.ENABLE = 1;
	while(SERCOM5->SPI.SYNCBUSY.bit.ENABLE);

	/* PIN MUX */
	PORT->Group[1].PINCFG[16].bit.PMUXEN = 1;
	PORT->Group[1].PINCFG[22].bit.PMUXEN = 1;
	PORT->Group[1].PINCFG[23].bit.PMUXEN = 1;

	PORT->Group[1].PMUX[8].bit.PMUXE  = 0x2;
	PORT->Group[1].PMUX[11].bit.PMUXE = 0x3;
	PORT->Group[1].PMUX[11].bit.PMUXO = 0x3;
}

void spi_send(uint8_t data)
{
	while(!SERCOM5->SPI.INTFLAG.bit.DRE);
	SERCOM5->SPI.DATA.reg = data;
}

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
	clock_init();
	sercom_spi_init();
	while(1)
	{
		for (uint8_t a=0; a<0xFF; a++)
		{
			spi_send(a);
		}
	}
}
