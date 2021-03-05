/*
 * Author: Adithya Yuri
 *
 * Baremetal code for Non-Volatile memory and SERCOM USART
 * This example can be a base to build low footprint bootloaders
 */

#include "sam.h"

void blinkTheDamnLED(void);
void uartInit(void);
void configGCLK1(void);
void nvm_erase_row(const uint32_t row_address);
void nvm_write_buffer(const uint32_t destination_address, const uint8_t *buffer, uint16_t length);
void nvmTest(void);

uint16_t a;
#define SERCOM_GCLK		16000000UL
#define SERCOM_BAUD	115200
#define BAUD_VAL	(65536.0*(1.0-((float)(16.0*(float)SERCOM_BAUD)/(float)SERCOM_GCLK)))
#define NVM_MEMORY        ((volatile uint16_t *)FLASH_ADDR)

volatile uint8_t nvm_dummy[64] = {
	0xb6, 0x63, 0x2a, 0xd4, 0x86, 0x28, 0x33, 0x8f, 0xf9, 0xad, 0xbc, 0x74, 0x76, 0x85, 0x72, 0x56,
	0x8e, 0xaa, 0x07, 0x20, 0xe3, 0xf3, 0x11, 0x8b, 0xe8, 0x34, 0x5a, 0xb1, 0x02, 0x60, 0xde, 0x3f,
	0xe7, 0xec, 0x75, 0x9c, 0x0c, 0xe1, 0xa7, 0x81, 0x58, 0x67, 0x50, 0xeb, 0xa5, 0x4f, 0x72, 0xf1,
	0x4f, 0xcb, 0x48, 0xa8, 0x0b, 0x06, 0x9f, 0xae, 0x54, 0xd5, 0xdf, 0x5e, 0x74, 0xda, 0xc2, 0x2b
	};

void blinkTheDamnLED()
{
	PORT->Group[1].DIR.reg = 1<<10;
	uint8_t b=0;
	while (1)
	{
		PORT->Group[1].OUTTGL.reg = 1<<10;
		//for (a=0; a<3000;a++);

		while (!SERCOM3->USART.INTFLAG.bit.DRE);
		SERCOM3->USART.DATA.reg = b;

		while(!SERCOM3->USART.INTFLAG.bit.RXC);
		b=((uint8_t)(SERCOM3->USART.DATA.reg & 0x00FF));
		b++;
	}
}

void configGCLK1()
{
	GCLK->GENCTRL[1].reg = GCLK_GENCTRL_GENEN | GCLK_GENCTRL_IDC | GCLK_GENCTRL_SRC_OSC16M;
	while(GCLK->SYNCBUSY.bit.GENCTRL);

	/* Enable clock for SERCOM 3 */
	GCLK->PCHCTRL[21].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1;
	MCLK->APBCMASK.bit.SERCOM3_ = 1;
}

void uartInit()
{
	/* Initialize GCLK clock for UART module */
	configGCLK1();
	/* Mux the TX(PA22) and RX(PA23) pins for SERCOM3 */
	PORT->Group[0].PINCFG[22].bit.PMUXEN = 1;
	PORT->Group[0].PMUX[11].bit.PMUXE = 0x02;

	PORT->Group[0].PINCFG[23].bit.PMUXEN = 1;
	PORT->Group[0].PMUX[11].bit.PMUXO = 0x02;

	// PORT->Group[0].PMUX[11].reg = 0x0202;

	SERCOM3->USART.CTRLA.reg = SERCOM_USART_CTRLA_MODE(1) | SERCOM_USART_CTRLA_DORD | SERCOM_USART_CTRLA_RXPO(1) | SERCOM_USART_CTRLA_TXPO(0);
	SERCOM3->USART.CTRLB.reg = SERCOM_USART_CTRLB_RXEN | SERCOM_USART_CTRLB_TXEN | SERCOM_USART_CTRLB_CHSIZE(0);
	while (SERCOM3->USART.SYNCBUSY.bit.CTRLB);
	SERCOM3->USART.BAUD.reg = BAUD_VAL;
	SERCOM3->USART.CTRLA.bit.ENABLE = 1;
}

void nvm_erase_row(const uint32_t row_address)
{
	/* Check if the module is busy */
	while(!NVMCTRL->INTFLAG.bit.READY);
	/* Clear error flags */
	NVMCTRL->STATUS.reg &= ~NVMCTRL_STATUS_MASK;
	/* Set address and command */
	NVMCTRL->ADDR.reg  = (uintptr_t)&NVM_MEMORY[row_address / 4];
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
	while(!NVMCTRL->INTFLAG.bit.READY);
}

void nvm_write_buffer(const uint32_t destination_address, const uint8_t *buffer, uint16_t length)
{
	/* Check if the module is busy */
	while(!NVMCTRL->INTFLAG.bit.READY);

	/* Erase the page buffer before buffering new data */
	NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_PBC | NVMCTRL_CTRLA_CMDEX_KEY;

	/* Check if the module is busy */
	while(!NVMCTRL->INTFLAG.bit.READY);

	/* Clear error flags */
	NVMCTRL->STATUS.reg &= ~NVMCTRL_STATUS_MASK;

	uint32_t nvm_address = destination_address / 2;

	/* NVM _must_ be accessed as a series of 16-bit words, perform manual copy
	 * to ensure alignment */
	for (uint16_t k = 0; k < length; k += 2)
	{
		uint16_t data;
		/* Copy first byte of the 16-bit chunk to the temporary buffer */
		data = buffer[k];
		/* If we are not at the end of a write request with an odd byte count,
		 * store the next byte of data as well */
		if (k < (length - 1)) {
			data |= (buffer[k + 1] << 8);
		}
		/* Store next 16-bit chunk to the NVM memory space */
		NVM_MEMORY[nvm_address++] = data;
	}
	while(!NVMCTRL->INTFLAG.bit.READY);
}

void nvmTest()
{
	//nvm_erase_row(0x20000);
	nvm_erase_row(0xB00);
	nvm_write_buffer(0xB00,nvm_dummy,64);

	nvm_erase_row(0xc00);
	nvm_write_buffer(0xc00,nvm_dummy,64);

	//nvm_write_buffer(0x20000,nvm_dummy,64);
}

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
	NVMCTRL->CTRLB.bit.CACHEDIS = 1;
	NVMCTRL->CTRLB.bit.MANW = 0;
    /* Enabling USART SERCOM module */
	uartInit();
	/* Changing System clock to 16MHz */
	OSCCTRL->OSC16MCTRL.bit.FSEL = 0x3;
	nvmTest();
	blinkTheDamnLED();
    while(1){
        /* Dummy loop */
    }
}

