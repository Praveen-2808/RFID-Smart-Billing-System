/*
 * init.c
 * -----------------------------------------------------------------------
 * Master hardware initialisation for the microcontroller side of the
 * project. Brings up the LCD, keypad, both UARTs, and configures the
 * three external interrupt pins (Entry / Delete / Exit push buttons)
 * along with the Vectored Interrupt Controller (VIC) entries that route
 * each interrupt to its ISR.
 * -----------------------------------------------------------------------
 */
#include<lpc21xx.h>
#include"lcd.h"
#include"kpm.h"
#include"project_defines.h"
#include"interrupt.h"
#include"types.h"
#include"uart0_uart1.h"
void init()
{
		/* Configure P0.2 as an output (used to drive an indicator/buzzer
		 * pin, e.g. when a card is scanned - see IOSET0/IOCLR0 usage in
		 * major_app_ucside.c). */
		IODIR0=1<<2;
        InitLCD();
        Initkpm();
		Init_UART0();
		Init_UART1();
      //  PINSEL0&=(u32)~3<<6|(u32)~3<<14;
        /* Clear PINSEL1 bits 0-1 (part of configuring EINT0 pin function). */
        PINSEL1&=(u32)~3<<0;
        /* Route the physical pins to their EINT1/EINT2 alternate function. */
        PINSEL0|=EINT1_IP_PIN|EINT2_IP_PIN;
        /* Route the physical pin to its EINT0 alternate function. */
        PINSEL1|=EINT0_IP_PIN;
        /* Assign all VIC channels as IRQ (not FIQ). */
        VICIntSelect=0;
        /* Enable the EINT0, EINT1 and EINT2 interrupt channels in the VIC. */
        VICIntEnable|=1<<EINT0_VIC_CHNO|1<<EINT1_VIC_CHNO|1<<EINT2_VIC_CHNO;
	/* Assign EINT0 a vectored slot (slot 2) and point it at eint0_isr. */
	VICVectCntl2=1<<5|EINT0_VIC_CHNO;
	VICVectAddr2=(u32)eint0_isr;
	/* Assign EINT1 a vectored slot (slot 3) and point it at eint1_isr. */
	VICVectCntl3=1<<5|EINT1_VIC_CHNO;
	VICVectAddr3=(u32)eint1_isr;
	/* Assign EINT2 a vectored slot (slot 4) and point it at eint2_isr. */
	VICVectCntl4=1<<5|EINT2_VIC_CHNO;
	VICVectAddr4=(u32)eint2_isr;
				
	/* Clear any pending external interrupt flags. */
	EXTINT=0;
	/* Configure EINT0/1/2 as edge-sensitive (rather than level-sensitive). */
	EXTMODE=(1<<0)|(1<<1)|(1<<2);
	/* Configure EINT0/1/2 to trigger on falling edge (active-low polarity). */
	EXTPOLAR=0;
				
}

