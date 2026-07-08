/*
 * uart0_uart1.c
 * -----------------------------------------------------------------------
 * Driver for the LPC21xx on-chip UART0 and UART1 peripherals.
 * UART0 is used to communicate with the PC-side application; UART1 is
 * used to communicate with the RFID card reader / keypad module.
 * Both are configured for 9600 baud, 8 data bits, receive interrupt
 * enabled, and hooked into the VIC to call uart0_isr / uart1_isr on
 * incoming data (see interrupt_isr.c).
 * -----------------------------------------------------------------------
 */
#include<Lpc21xx.h>
#include"types.h"
#include"uart_defines.h"
#include"project_defines.h"
#include"interrupt.h"
#include"uart0_uart1.h"

void Init_UART0(void)
{
	//cfg p0.0&p0.1 as TXD0&RXD0
	PINSEL0&=~(15<<0);
	//PINSEL0&=~(0xffffffff<<0);
	//update pin selection
	PINSEL0|=TxD0_PIN_FUNC|RxD0_PIN_FUNC;
	//cfg UXLCR for
	/* Set DLAB=1 to allow writing the baud rate divisor, and select
	 * 8-bit word length. */
	U0LCR|=(1<<DLAB_BIT)|WOR_LEN_SEL_;
	//cfg speed/freq/baudrate
	/* Load the 16-bit baud rate divisor (high byte then low byte). */
	U0DLM=DIVISOR>>8;
	U0DLL=DIVISOR;
	//Reset DLAB bit to start comm
	/* Clear DLAB so THR/RBR are accessible again for normal tx/rx. */
	U0LCR&=~(1<<DLAB_BIT);
	/* Enable "Receive Data Available" interrupt. */
	U0IER=(1<<0);
	/* Enable UART0's channel in the VIC and hook uart0_isr as its
	 * vectored interrupt handler. */
	VICIntEnable|=1<<UART0_VIC_CHNO;
	VICVectCntl1=1<<5|UART0_VIC_CHNO;
	VICVectAddr1=(u32)uart0_isr;
}

void U0_TxByte(u8 byte)
{
	//place byte to transmit in to UxTHR
	U0THR=byte;
	//monitor until transmission to complete
	/* Poll the Transmitter Empty (TEMT) bit in UxLSR until the byte has
	 * fully shifted out. */
	while(((U0LSR>>TEMT_BIT)&1)==0);
}

void U0_Txstring(char *p)
{
	/* Transmit each character of the null-terminated string in turn. */
	while(*p)
	{
		U0_TxByte(*p++);
	}
}

void Init_UART1(void)
{
	//cfg p0.0&p0.1 as TXD0&RXD0
	PINSEL0&=~(15<<16);
	//update pin selection
	PINSEL0|=TxD1_PIN_FUNC|RxD1_PIN_FUNC;
	//cfg UXLCR for
	/* Set DLAB=1 to allow writing the baud rate divisor, and select
	 * 8-bit word length. */
	U1LCR|=(1<<DLAB_BIT)|WOR_LEN_SEL_;
	//cfg speed/freq/baudrate
	/* Load the 16-bit baud rate divisor (high byte then low byte). */
	U1DLM=DIVISOR>>8;
	U1DLL=DIVISOR;
	//Reset DLAB bit to start comm
	/* Clear DLAB so THR/RBR are accessible again for normal tx/rx. */
	U1LCR&=~(1<<DLAB_BIT);
	/* Enable "Receive Data Available" interrupt. */
	U1IER=(1<<0);
	/* Enable UART1's channel in the VIC and hook uart1_isr as its
	 * vectored interrupt handler. */
	VICIntEnable|=1<<UART1_VIC_CHNO;
	VICVectCntl0=1<<5|UART1_VIC_CHNO;
	VICVectAddr0=(u32)uart1_isr;
}

void U1_TxByte(u8 byte)
{
	//place byte to transmit in to UxTHR
	U1THR=byte;
	//monitor until transmission to complete
	/* Poll the Transmitter Empty (TEMT) bit in UxLSR until the byte has
	 * fully shifted out. */
	while(((U1LSR>>TEMT_BIT)&1)==0);
}

void U1_Txstring(char *p)
{
	/* Transmit each character of the null-terminated string in turn.
	 * NOTE: this calls U0_TxByte (UART0), not U1_TxByte - kept as-is,
	 * not modified, since changing it would alter behaviour. */
	while(*p)
	{
		U0_TxByte(*p++);
	}
}

