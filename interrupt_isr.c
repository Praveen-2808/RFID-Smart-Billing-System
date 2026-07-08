/*
 * interrupt_isr.c
 * -----------------------------------------------------------------------
 * Interrupt Service Routines (ISRs) for the two UARTs and the three
 * external-interrupt push buttons (Entry / Delete / Exit).
 *
 * UART1 ISR: receives RFID card data framed between STX (0x02) and
 *            ETX (0x03) bytes into `buff`.
 * UART0 ISR: receives item/response data from the PC side, framed
 *            between '#' and '$' characters, into `rx_buff`.
 * EINTx ISRs: simply set a flag for the main loop to notice and act on,
 *             then acknowledge/clear the interrupt.
 * -----------------------------------------------------------------------
 */
#include<lpc21xx.h>
#include<string.h>
#include"types.h"
/* Shared buffers/flags defined in major_app_ucside.c, used to hand data
 * from these interrupt handlers to the main application loop. */
extern s8 buff[30],rx_buff[50];
extern s32 entry_flag,delete_flag,exit_flag,scan_flag,recv_flag;
s8 data;      /* last byte received on either UART */
s32 index;    /* current write position within buff / rx_buff */

/* UART1 receive interrupt: assembles bytes coming from the RFID/keypad
 * module into `buff`, framed by STX(0x02)...ETX(0x03). */
void uart1_isr(void) __irq
{
	data=U1RBR;
	if(data==0x02)	
	{
		/* Start of a new frame: reset the buffer index and clear buff. */
		index=0;
		memset(buff,0,sizeof(buff));
	}
	else if(data==0x03)
	{
		/* End of frame: null-terminate and signal the main loop that a
		 * complete scan is ready via scan_flag. */
		buff[index]='\0';
		scan_flag=1;
	}
	else
	{
		/* Regular data byte: append to the buffer. */
	 	buff[index++]=data;
	}
	/* Acknowledge interrupt to the VIC so further interrupts can be serviced. */
	VICVectAddr=0;
}
/* UART0 receive interrupt: assembles bytes coming from the PC side into
 * `rx_buff`, framed by '#'...'$'. */
void uart0_isr(void) __irq
{
	data=U0RBR;
	if(data=='#')
	{
		/* Start of a new frame: reset the buffer index and clear rx_buff. */
		index=0;
		memset(rx_buff,0,sizeof(rx_buff));
	}		
	else if(data=='$')
	{
		/* End of frame: null-terminate and signal the main loop that a
		 * complete PC response has been received via recv_flag. */
		rx_buff[index]='\0';
		recv_flag=1;
	}
	else
	{
		/* Regular data byte: append to the buffer. */
		rx_buff[index++]=data;
	}
	/* Acknowledge interrupt to the VIC so further interrupts can be serviced. */
	VICVectAddr=0;
}
/* External interrupt 0: "Entry" button pressed - begin a shopping session. */
void eint0_isr()__irq
{
	entry_flag=1;
	/* Clear the EINT0 pending flag. */
	EXTINT=1<<0;
	VICVectAddr=0;
}
/* External interrupt 1: "Delete" button pressed - remove the last scanned item. */
void eint1_isr()__irq
{	
	delete_flag=1;
	/* Clear the EINT1 pending flag. */
	EXTINT=1<<1;
	VICVectAddr=0;
}
/* External interrupt 2: "Exit/Checkout" button pressed - proceed to payment. */
void eint2_isr()__irq
{
	exit_flag=1;
	/* Clear the EINT2 pending flag. */
	EXTINT=1<<2;
	VICVectAddr=0;	
}
