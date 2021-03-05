/*
 * Author: Adithya Yuri
 *
 * Baremetal test code for L21 AES peripheral in ECB mode
 * This example can be a base to build low footprint encrypted bootloaders
 */

#include "sam.h"

void blinkTheDamnLED(void);
void uartInit(void);
void configGCLK1(void);
void uart_send_char(uint8_t data8);
void uart_get_char(uint8_t data8);
void configAES(void);
void AES_putdata(void);
void AES_getdata(void);
void uart_send_string(uint8_t *str, uint8_t len);
void delay(uint32_t d);


#define SERCOM_GCLK		16000000UL
#define SERCOM_BAUD	115200
#define BAUD_VAL	(65536.0*(1.0-((float)(16.0*(float)SERCOM_BAUD)/(float)SERCOM_GCLK)))

//--------------------------------------------------------------------------
volatile uint32_t actual_AES_output[4];

volatile uint8_t x=0;
volatile uint8_t y=0;

uint32_t ref_plain_text[4] = {
	0xe2bec16b,
	0x969f402e,
	0x117e3de9,
	0x2a179373
};

/** Reference ECB cipher data. */
uint32_t ref_cipher_text_ecb[4] = {
	0xb47bd73a,
	0x60367a0d,
	0xf3ca9ea8,
	0x97ef6624
};

/** Cipher 128 bits key array. */
volatile uint32_t key128[4] = {
	0x16157e2b,
	0xa6d2ae28,
	0x8815f7ab,
	0x3c4fcf09
};
//--------------------------------------------------------------------------
void delay(uint32_t d)
{
	for (uint32_t a=0; a<d; a++)
	{
	}
}
void blinkTheDamnLED()
{
	PORT->Group[1].DIR.reg = 1<<10;
	uint32_t a=0;
	for(uint8_t i= 10; i<10; i++)
	{
		PORT->Group[1].OUTTGL.reg = 1<<10;
		for (a=0; a<3000;a++);
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

void uart_send_char(uint8_t data8)
{
	while (!SERCOM3->USART.INTFLAG.bit.DRE);
	SERCOM3->USART.DATA.reg = data8;
}

void uart_get_char(uint8_t data8)
{
	while(!SERCOM3->USART.INTFLAG.bit.RXC);
	data8 = ((uint8_t)(SERCOM3->USART.DATA.reg & 0x00FF));
}

void configAES()
{
	MCLK->APBCMASK.bit.AES_ = 1;

	AES->DBGCTRL.bit.DBGRUN = 1; //Run AES while debug

	AES->CTRLA.bit.AESMODE		= AES_CTRLA_AESMODE(0x0); //ECB-0x1 , CBC-0x1
	AES->CTRLA.bit.KEYSIZE		= 0x0; // Key size 128 bits
	AES->CTRLA.bit.CIPHER		= 0x1; //Encryption mode
	AES->CTRLA.bit.STARTMODE	= 0x0; //0-Manual mode 1-AUTO mode
	AES->CTRLA.bit.CTYPE		= 0xF; //Enable all CTYPE stuff
	AES->CTRLA.bit.ENABLE		= 0x1; //Enable the thingy

	AES->KEYWORD[0].reg			= key128[0];
	AES->KEYWORD[1].reg			= key128[1];
	AES->KEYWORD[2].reg			= key128[2];
	AES->KEYWORD[3].reg			= key128[3];

}

void AES_putdata(void)
{
	AES->DATABUFPTR.bit.INDATAPTR = 0x0;
	for (x=0; x<4; x++)
	{
		AES->DATABUFPTR.bit.INDATAPTR = x;
		AES->INDATA.reg = ref_plain_text[x];
	}
	AES->CTRLB.bit.START = 0x1;
}

void AES_getdata(void)
{
	AES->DATABUFPTR.bit.INDATAPTR = 0x0;
	while(!AES->INTFLAG.bit.ENCCMP);

	AES->INTFLAG.bit.ENCCMP = 0x1;

	for (y=0; y<4; y++)
	{
		AES->DATABUFPTR.bit.INDATAPTR = y;
		actual_AES_output[y] = AES->INDATA.reg;
	}
}

void uart_send_string(uint8_t *str, uint8_t len)
{
	for (uint8_t z; z<len; z++)
	{
		uart_send_char(str[z]);
	}
}

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();

	OSCCTRL->OSC16MCTRL.bit.FSEL = 0x3;

	blinkTheDamnLED();
	uartInit();
	uart_send_string("Started the example \n\r", 24);
	delay(100);
	uart_send_string("AES ECB of course \n\r", 22);
	configAES();
	AES->CTRLB.bit.NEWMSG = 0x1;
	AES_putdata();
	delay(100);
	uart_send_string("AES data has been fed \n\r", 26);
	AES->CTRLB.bit.NEWMSG = 0x0;
	AES_getdata();
	delay(100);
	uart_send_string("Got the encrypted data from the AES module \n\r", 47);
	delay(100);
	uart_send_string("Lets check the data now \n\r", 28);

	if (actual_AES_output[0]==ref_cipher_text_ecb[0] &&
		actual_AES_output[1]==ref_cipher_text_ecb[1] &&
		actual_AES_output[2]==ref_cipher_text_ecb[2] &&
		actual_AES_output[3]==ref_cipher_text_ecb[3]
			)
	{
		delay(100);
		uart_send_string("I think it works ! ! ! !", 24);
	}
	else
	{
		delay(100);
		uart_send_string("! Hopeless stuff !", 18);
	}
	while(1);
}
