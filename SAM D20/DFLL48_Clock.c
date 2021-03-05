/*
 * Author: Adithya Yuri
 *
 * Baremetal code to setup DFLL 48MHz as system clock. (XOSC32K-External 32k crystal is used as reference)
 */

#include "sam.h"
#include <stdbool.h>

void lock_init(void)
{
	/* Enable the XOSC32K and set the start up time */
	SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_STARTUP(6) | SYSCTRL_XOSC32K_EN32K | SYSCTRL_XOSC32K_XTALEN | SYSCTRL_XOSC32K_ENABLE;
	/* Wait for the XOSC32K is stable and ready */
	while(!SYSCTRL->PCLKSR.bit.XOSC32KRDY);
	/* Enable the Generic Clock 1 and Configure the OSC32K as Clock Source for it*/
	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID_GCLK1 | GCLK_GENCTRL_SRC_XOSC32K | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_IDC;
	/* Wait for the synchronization between clock domain is complete */
	while(GCLK->STATUS.bit.SYNCBUSY);
	/* Enable the DFLL and set the operation mode as closed loop */
	SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE | SYSCTRL_DFLLCTRL_MODE;
	/* Wait for the synchronization between clock domain is complete */
	while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
	/* Load the Multiply factor, Coarse Step and fine Step for DFLL */
	SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(0x1F/4) | SYSCTRL_DFLLMUL_FSTEP(0xFF/4) | SYSCTRL_DFLLMUL_MUL(1465);
	/* Enable the Generic Clock 1 for DFLL48 as Reference */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_ID(0);
	while(!SYSCTRL->PCLKSR.bit.DFLLLCKF);
	/* Set the NVM Read Wait States to 1, Since the operating frequency 48 MHz */
	NVMCTRL->CTRLB.bit.RWS = 1;
	/*  Enable the Generic Clock 0 and Configure the DFLL as Clock Source for it*/
	GCLK->GENCTRL.reg = GCLK_GENCTRL_ID_GCLK0 | GCLK_GENCTRL_SRC_DFLL48M | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_IDC;

	/* Enable the APB clock for the SERCOM2 module */
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM2;
	/* Enable the Generic clock 0 for the SERCOM2 module */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(SERCOM2_GCLK_ID_CORE);
}

int main(void)
{
	clock_init();

    while (1)
    {
	    /* Dummy loop */
    }
}
