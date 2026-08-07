/*
 * project_defines.h
 * -----------------------------------------------------------------------
 * Board/peripheral-specific constants for configuring the external
 * interrupt pins (EINT0/1/2) via PINSEL registers, and the VIC
 * (Vectored Interrupt Controller) channel numbers used to hook up
 * each interrupt source on the LPC21xx microcontroller.
 * -----------------------------------------------------------------------
 */

/* PINSEL bit patterns that route the physical pins to their EINTx
 * (external interrupt) alternate function instead of GPIO. */
#define EINT0_IP_PIN 0x00400001
#define EINT1_IP_PIN 0x000000C0
#define EINT2_IP_PIN 0x0000C000

/* VIC channel numbers for each peripheral/interrupt source, used when
 * configuring VICVectCntlx / VICIntEnable registers. */
#define UART0_VIC_CHNO 6
#define UART1_VIC_CHNO 7
#define EINT0_VIC_CHNO 14
#define EINT1_VIC_CHNO 15
#define EINT2_VIC_CHNO 16
