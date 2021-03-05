/*
 * Author: Adithya Yuri
 *
 * Code is setup to receive 8bytes and transfer it to RAM through DMA
 */

#include "sam.h"
#include <stdbool.h>

#define COMPILER_ALIGNED(a)        __attribute__((__aligned__(a)))
#define BUF_SIZE	8

uint8_t buffer[BUF_SIZE];
bool rx_done = false;

COMPILER_ALIGNED(16)
volatile DmacDescriptor main_desc_per_to_mem, wb_desc_per_to_mem;

void DMAC_Handler(void)
{
	DMAC->CHID.bit.ID = 0;
	DMAC->CHINTFLAG.bit.TCMPL = 1;
	rx_done = true;
}

void pin_set_peripheral_function(uint32_t pinmux)
{
	uint8_t port = (uint8_t)((pinmux >> 16)/32);
	PORT->Group[port].PINCFG[((pinmux >> 16) - (port*32))].bit.PMUXEN = 1;
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg &= ~(0xF << (4 * ((pinmux >> 16) & 0x01u)));
	PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg |= (uint8_t)((pinmux & 0x0000FFFF) << (4 * ((pinmux >> 16) & 0x01u)));
}

void spi_init(void)
{
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0;
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(0) | GCLK_CLKCTRL_ID_SERCOM0_CORE;

	pin_set_peripheral_function(PINMUX_PA04D_SERCOM0_PAD0);
	pin_set_peripheral_function(PINMUX_PA05D_SERCOM0_PAD1);
	pin_set_peripheral_function(PINMUX_PA06D_SERCOM0_PAD2);
	pin_set_peripheral_function(PINMUX_PA07D_SERCOM0_PAD3);

	SERCOM0->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_DIPO(0) | SERCOM_SPI_CTRLA_DOPO(1) | SERCOM_SPI_CTRLA_MODE_SPI_SLAVE;
	SERCOM0->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN | SERCOM_SPI_CTRLB_SSDE | SERCOM_SPI_CTRLB_PLOADEN;
	while(SERCOM0->SPI.SYNCBUSY.bit.CTRLB);
	SERCOM0->SPI.CTRLA.bit.ENABLE = 1;
	while(SERCOM0->SPI.SYNCBUSY.bit.ENABLE);
}

void dma_init(void)
{
	main_desc_per_to_mem.BTCTRL.reg = DMAC_BTCTRL_DSTINC | DMAC_BTCTRL_BEATSIZE_BYTE | DMAC_BTCTRL_VALID;
	main_desc_per_to_mem.BTCNT.reg = BUF_SIZE;
	main_desc_per_to_mem.SRCADDR.reg = (uint32_t)&SERCOM0->SPI.DATA.reg;
	main_desc_per_to_mem.DSTADDR.reg = (uint32_t)(buffer + sizeof(buffer));
	main_desc_per_to_mem.DESCADDR.reg = 0x00000000;

	PM->AHBMASK.reg |= PM_AHBMASK_DMAC;
	PM->APBBMASK.reg |= PM_APBBMASK_DMAC;
	DMAC->CTRL.bit.DMAENABLE = 0;
	DMAC->CTRL.bit.SWRST = 1;

	DMAC->BASEADDR.reg = (uint32_t)&main_desc_per_to_mem;
	DMAC->WRBADDR.reg = (uint32_t)&wb_desc_per_to_mem;

	DMAC->CHID.bit.ID = 0;
	DMAC->CHCTRLB.reg = DMAC_CHCTRLB_TRIGACT_BEAT | DMAC_CHCTRLB_TRIGSRC(0x01) | DMAC_CHCTRLB_LVL_LVL0;
	DMAC->CHINTENSET.bit.TCMPL = 1;
	DMAC->CHINTFLAG.bit.TCMPL = 1;
	DMAC->CHCTRLA.reg = DMAC_CHCTRLA_ENABLE;

	DMAC->CTRL.reg = DMAC_CTRL_LVLEN0 | DMAC_CTRL_DMAENABLE;
	NVIC_EnableIRQ(DMAC_IRQn);
}

int main(void)
{
	SYSCTRL->OSC8M.bit.PRESC = 0;
	spi_init();
    dma_init();
	__enable_irq();

    while (1)
    {
        // Dummy loop
    }
}
