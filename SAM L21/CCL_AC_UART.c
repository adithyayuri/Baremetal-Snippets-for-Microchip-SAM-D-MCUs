/*
 * Author: Adithya Yuri
 *
 * Baremetal code for CCL (Configurable Custom Logic)
 */


#include "sam.h"
#include "stdbool.h"

void blinkTheDamnLED(void);
void analogComparator0Conf(void);
void ACPinMuxConf(void);
void configGCLK1(void);
void uartInit(void);
void uart_send_char(uint8_t data8);
uint8_t uart_get_char();
void uart_send_string(uint8_t *str, uint8_t len);
void ccl0Conf(void);
void delay(uint32_t dly);

#define SERCOM_GCLK		16000000UL
#define SERCOM_BAUD	115200
#define BAUD_VAL	(65536.0*(1.0-((float)(16.0*(float)SERCOM_BAUD)/(float)SERCOM_GCLK)))

void blinkTheDamnLED()
{
	// no big deal here
	PORT->Group[1].DIR.reg = 1<<10;
	uint32_t a=0;
	while(1)
	{
		PORT->Group[1].OUTTGL.reg = 1<<10;
		for (a=0; a<80000;a++);
	}
}

void analogComparator0Conf()
{
	// Re-enable AC peripheral in APBD
	MCLK->APBDMASK.bit.AC_ = true;
	// Give clock to AC peripheral (FYI - it does not need clock in all modes(SYNC modes, edge detect stuff, filter stuff )
	GCLK->PCHCTRL[31].reg = GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;

	// Make AC output ASYNCRONOUS to main clock
	AC->COMPCTRL[0].bit.OUT		= AC_COMPCTRL_OUT_ASYNC_Val;
	// Not gonna use the filter
//	AC->COMPCTRL[0].bit.FLEN	= AC_COMPCTRL_FLEN_OFF_Val;
	// Gonna use the filter - changed my mind !
	AC->COMPCTRL[0].bit.FLEN	= AC_COMPCTRL_FLEN_MAJ3_Val;
	// Set the hysteresis level, the more the better for avoiding edge oscillations (can cause HF oscillations at both edges)
	AC->COMPCTRL[0].bit.HYST	= AC_COMPCTRL_HYST_HYST90_Val;
	// Gotta enable the hysteresis
	AC->COMPCTRL[0].bit.HYSTEN	= true;
	// Set the speed to max
	AC->COMPCTRL[0].bit.SPEED	= AC_COMPCTRL_SPEED_HIGH_Val;
	// set Muxpos to PIN0
	AC->COMPCTRL[0].bit.MUXPOS	= AC_COMPCTRL_MUXPOS_PIN0_Val;
	// set ref level from bandgap
	AC->COMPCTRL[0].bit.MUXNEG	= AC_COMPCTRL_MUXNEG_BANDGAP_Val;

	// Enable comparator
	AC->COMPCTRL[0].bit.ENABLE	= true;
	//enable Run while debug
	AC->DBGCTRL.bit.DBGRUN		= AC_DBGCTRL_MASK;
	// Enable peripheral
	AC->CTRLA.bit.ENABLE		= true;

	// Wait for clock domains to sync'ed up
	while(AC->SYNCBUSY.bit.ENABLE);
	// Wait for comparator 0 to be ready - probably necessary
	while(!AC->STATUSB.bit.READY0);

	// Mux IO pins
	ACPinMuxConf();
}

void ACPinMuxConf()
{
	// Muxin pins - notin fancy
	PORT->Group[0].PINCFG[4].bit.PMUXEN		= true;
	PORT->Group[0].PMUX[2].bit.PMUXE		= 0x1; //B function- PA04 for AC input

	PORT->Group[0].PINCFG[12].bit.PMUXEN	= true;
	PORT->Group[0].PMUX[6].bit.PMUXE		= 0x7; //H function- PA12 for AC output
}

void configGCLK1()
{
	// Enable GCKL1 with internal 16M clock
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

uint8_t uart_get_char()
{
	while(!SERCOM3->USART.INTFLAG.bit.RXC);
	return ((uint8_t)(SERCOM3->USART.DATA.reg & 0x00FF));
}

void uart_send_string(uint8_t *str, uint8_t len)
{
	for (uint8_t z; z<len; z++)
	{
		uart_send_char(str[z]);
	}
}

void ccl0Conf()
{
	/* realizing a not gate
	2	1	0	r
	0	0	0	1
	0	0	1	0
	0	1	0	1
	0	1	1	0
	1	0	0	1
	1	0	1	0
	1	1	0	1
	1	1	1	0

	2	1	0	r
	x	x	0	1
	x	x	1	0
	x	x	0	1
	x	x	1	0
	x	x	0	1
	x	x	1	0
	x	x	0	1
	x	x	1	0

	TRUTH = 0b01010101
	*/
	// Re-enable CCL peripheral in APBD
	MCLK->APBDMASK.bit.CCL_		= true;
	// Give clock to AC peripheral (FYI - it does not need clock in all modes(SYNC modes, edge detect stuff, filter stuff )
	GCLK->PCHCTRL[34].reg		= GCLK_PCHCTRL_GEN_GCLK0 | GCLK_PCHCTRL_CHEN;

	// Truth tables values, as computated above
	CCL->LUTCTRL[0].bit.TRUTH	= 0b01010101;
	// Internally take AC-out as input for CCL
	CCL->LUTCTRL[0].bit.INSEL0	= CCL_LUTCTRL_INSEL0_AC_Val;
	// Dummy
	CCL->LUTCTRL[0].bit.INSEL1	= 0x0;
	// Dummy
	CCL->LUTCTRL[0].bit.INSEL2	= 0x0;
	// Disabled the filter
	CCL->LUTCTRL[0].bit.FILTSEL	= CCL_LUTCTRL_FILTSEL_DISABLE_Val;
	// Disabled EDGE detection
	CCL->LUTCTRL[0].bit.EDGESEL	= false;
	// Enable LUT0
	CCL->LUTCTRL[0].bit.ENABLE	= true;

	// Enable CCL peripheral
	CCL->CTRL.bit.ENABLE		= true;

	//PA07 CCL0 out
	//MUXING out pin
	PORT->Group[0].PINCFG[7].bit.PMUXEN	= true;
	PORT->Group[0].PMUX[3].bit.PMUXO	= 0x8;
}

void delay(uint32_t dly)
{
	for (uint32_t u=0;u<dly;u++);
}

int main(void)
{
    /* Initialize the SAM system */
    SystemInit();
	OSCCTRL->OSC16MCTRL.bit.FSEL = 0x3;
	uartInit();

	uart_send_string("EXAMPLE STARTED \r\n\r\n", sizeof("EXAMPLE STARTED \r\n\r\n"));
	delay(10000);
	uart_send_string("A external signal has to be fed into Analog Comparator(AC) pin PA04 \r\n", sizeof("A external signal has to be fed into Analog Comparator(AC) pin PA04 \r\n"));
	delay(10000);
	uart_send_string("This signal is compared with internal bandgap and AC output is routed to PA12 \r\n", sizeof("This signal is compared with internal bandgap and AC output is routed to PA12 \r\n"));
	delay(10000);
	uart_send_string("The AC output is internally routed to CCL(LUT0) and a not gate is configured \r\n", sizeof("The AC output is internally routed to CCL(LUT0) and a not gate is configured \r\n"));
	delay(10000);
	uart_send_string("The output of CCL LUT0 NOT gate is routed to pin PA07 \r\n", sizeof("The output of CCL LUT0 NOT gate is routed to pin PA07 \r\n"));
	delay(10000);
	uart_send_string("\r\n",sizeof("\r\n"));
	delay(10000);
	uart_send_string("Press Y to start example \r\n\r\n", sizeof("Press Y to start example \r\n\r\n"));

	while (!(uart_get_char()=='y'));

	delay(100);
	uart_send_string("Starting AC \r\n\r\n", sizeof("Starting AC \r\n\r\n"));
	analogComparator0Conf();
	delay(100);
	uart_send_string("Starting CCL \r\n\r\n", sizeof("Starting CCL \r\n\r\n"));
	ccl0Conf();
	delay(100);
	uart_send_string("Put a square wave(under 100khz with 2V pk-pk) in pin PA04\r\n", sizeof("Put a square wave(under 100khz with 2V pk-pk) in pin PA04\r\n"));
	delay(100);
	uart_send_string("Scope pins PA12(AC-OUT) and PA07(CCL-OUT) \r\n", sizeof("Scope pins PA12(AC-OUT) and PA07(CCL-OUT) \r\n"));
	delay(100);

	blinkTheDamnLED();

}
