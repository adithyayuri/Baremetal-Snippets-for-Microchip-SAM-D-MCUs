/*
 * Author: Adithya Yuri
 *
 * Baremetal code for clock init and SERCOM I2C Slave
 */

#include "sam.h"
#include <stdbool.h>

#define SLAVE_ADDR	0x12
#define OK			0
#define NOT_OK		-1
#define DATA_LENGTH	8


uint8_t tx_buff[8] = {0xFF, 2, 3, 4, 5, 6, 7, 8 };
uint8_t rx_buff[8], i = 0;
uint8_t status = OK;
uint8_t tx_buff_length = 0;
uint8_t rx_buff_length = 0;


void SERCOM2_Handler(void)
{
	/* Address Match Interrupt */
	if(SERCOM2->I2CS.INTFLAG.bit.AMATCH)
	{
		/* Clearing the Address Match interrupt Flag to send Address Match ACK */
		 SERCOM2->I2CS.INTFLAG.bit.AMATCH = 1;
	}
	/* Data Ready Interrupt */
	if (SERCOM2->I2CS.INTFLAG.bit.DRDY)
	{
		/* Check for Direction bit Set  for Master Read Operation */
		if(SERCOM2->I2CS.STATUS.bit.DIR)
		{
			/* Check the Transmit Buffer length */
			if(tx_buff_length >= DATA_LENGTH)
			{
				/* Send the NACK from the slave*/
				SERCOM2->I2CS.CTRLB.reg |= SERCOM_I2CS_CTRLB_ACKACT;
				SERCOM2->I2CS.CTRLB.reg |= SERCOM_I2CS_CTRLB_CMD(0x2);
				/* Clear the Buffer length */
				tx_buff_length = 0;
			}
			else
			{
				/* Transmit the byte from the slave to Master */
				SERCOM2->I2CS.DATA.reg = tx_buff[tx_buff_length++];
				/* Check the buffer length */
				if(tx_buff_length == DATA_LENGTH)
				{
					tx_buff_length = 0;
					/* Clear the Date Ready Interrupt Flag */
					SERCOM2->I2CS.INTFLAG.bit.DRDY = 1;
				}
			}
		}
		/* Master Write Operation */
		else
		{
			/* Check the Receive Buffer length */
			if(rx_buff_length >= DATA_LENGTH)
			{
				/* Send the NACK from the slave*/
				SERCOM2->I2CS.CTRLB.reg |= SERCOM_I2CS_CTRLB_ACKACT;
				SERCOM2->I2CS.CTRLB.reg |= SERCOM_I2CS_CTRLB_CMD(0x2);
				/* Clear the Buffer length */
				rx_buff_length = 0;
			}
			else
			{
				/* Receive the byte from the slave to Master */
				rx_buff[rx_buff_length++] = SERCOM2->I2CS.DATA.reg;
				/* Check the buffer length */
				if(rx_buff_length == DATA_LENGTH)
				{
					rx_buff_length = 0;
					/* Clear the Date Ready Interrupt Flag */
					SERCOM2->I2CS.INTFLAG.bit.DRDY = 1;
				}
			}
		}
	}
	/* Stop Received Interrupt */
	if (SERCOM2->I2CS.INTFLAG.bit.PREC)
	{
		/* Clear the Stop Received Interrupt Flag */
		SERCOM2->I2CS.INTFLAG.bit.PREC = 1;
		rx_buff_length = 0;
		tx_buff_length = 0;
	}
	/* Check for Bus Error, Transmit collision, SCL Timeout */
	if (SERCOM2->I2CS.STATUS.reg & (SERCOM_I2CS_STATUS_BUSERR | SERCOM_I2CS_STATUS_COLL | SERCOM_I2CS_STATUS_LOWTOUT))
	{
		status = NOT_OK;
	}
}

void clock_init(void)
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
}

void I2C_Slave_Init(void)
{
	/* Enable peripheral MUX for PA08 and PA09 as SERCOM2 I2C Pins */
	PORT->Group[0].PINCFG[8].bit.PMUXEN = 1;
	PORT->Group[0].PINCFG[9].bit.PMUXEN = 1;
	/* Configure the Peripheral Function as I2C for PA08 and PA09 pins */
	PORT->Group[0].PMUX[4].reg = PORT_PMUX_PMUXE(3) | PORT_PMUX_PMUXO(3);

	/* Enable the APB clock for the SERCOM2 module */
	PM->APBCMASK.reg |= PM_APBCMASK_SERCOM2;
	/* Enable the Generic clock 0 for the SERCOM2 module */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(SERCOM2_GCLK_ID_CORE);

	/* Enable the SMART mode for I2C Slave for ACK on Data Read*/
	SERCOM2->I2CS.CTRLB.reg	= SERCOM_I2CS_CTRLB_SMEN;
	/* Wait for the synchronization between clock domain is complete */
	while(SERCOM2->I2CS.STATUS.bit.SYNCBUSY);
	/* Load the slave address in the SERCOM2 address register */
	SERCOM2->I2CS.ADDR.reg	= SERCOM_I2CS_ADDR_ADDR(SLAVE_ADDR);
	/* Enable the I2C, Address Match, Data Ready and Stop Interrupt */
	SERCOM2->I2CS.INTENSET.reg	= SERCOM_I2CS_INTENSET_PREC | SERCOM_I2CS_INTENSET_AMATCH | SERCOM_I2CS_INTENSET_DRDY;
	/* Enable SERCOM2 module and set the mode as I2C Slave */
	SERCOM2->I2CS.CTRLA.reg	= SERCOM_I2CS_CTRLA_ENABLE | SERCOM_I2CS_CTRLA_MODE_I2C_SLAVE;
	/* Wait for the synchronization between clock domain is complete */
	while(SERCOM2->I2CS.STATUS.bit.SYNCBUSY);
	/* Enable the Vectored interrupt Control for the SERCOM2 module */
	NVIC_EnableIRQ(SERCOM2_IRQn);
	/* Set high level priority for SERCOM2 module */
	NVIC_SetPriority(SERCOM2_IRQn, 0);
}

int main(void)
{
	clock_init();
	I2C_Slave_Init();

    while (1)
    {
		/* Dummy loop */
    }
}
