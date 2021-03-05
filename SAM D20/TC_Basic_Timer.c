/*
 * Author: Adithya Yuri
 *
 * Timer generates interrupt on timer counter overflow and PA14 LED is toggled on handler
 */

#include "sam.h"

/* TC0 Handler */
void TC0_Handler(void)
{
	/* Clear overflow interrupt flag */
	TC0->COUNT16.INTFLAG.bit.OVF = 1;
	/* Toggle LED0 (PA14) */
	PORT->Group[0].OUTTGL.reg = (1u << 14);
}

/* Initialize TC0 */
void timer_init(void)
{
	/* Enable APB clock for TC0 */
	PM->APBCMASK.reg |= PM_APBCMASK_TC0;
	/* Enable GCLK for TC0 - Use GCLK GEN 0 as its source */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(0) | GCLK_CLKCTRL_ID_TC0_TC1;
	/* Enable TC0 */
	TC0->COUNT16.CTRLA.reg = TC_CTRLA_ENABLE;
	/* Wait for sync */
	while(TC0->COUNT16.STATUS.bit.SYNCBUSY);
	/* Enable TC0 overflow interrupt */
	TC0->COUNT16.INTENSET.bit.OVF = 1;
	/* Enable TC0 module interrupt - NVIC core interrupt */
	NVIC_EnableIRQ(TC0_IRQn);
}

int main(void)
{
	/* Set SAM D20 XPRO LED0 pin (PA14) as output */
	PORT->Group[0].DIRSET.reg = (1u << 14);

    timer_init();

    while (1)
    {
        /* Dummy loop */
    }
}
