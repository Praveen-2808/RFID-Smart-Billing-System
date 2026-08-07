/*
 * uart_defines.h
 * -----------------------------------------------------------------------
 * Clock and UART register configuration constants for the LPC21xx
 * microcontroller UART peripherals.
 * -----------------------------------------------------------------------
 */

#define FOSC 12000000            /* External crystal oscillator frequency: 12 MHz */
#define CCLK (FOSC*5)            /* CPU clock after PLL multiplication (x5) = 60 MHz */
#define PCLK (CCLK/4)            /* Peripheral clock (VPB divider = 4) = 15 MHz */
#define BAUD 9600                /* Desired UART baud rate */
#define DIVISOR (PCLK/(16*BAUD)) /* UART baud rate divisor loaded into U0DLM/U0DLL */

//pin connect block defines
#define TxD0_PIN_FUNC 0x00000001   /* PINSEL0 bits selecting P0.0 as TXD0 */
#define RxD0_PIN_FUNC 0x00000004   /* PINSEL0 bits selecting P0.1 as RXD0 */

#define TxD1_PIN_FUNC 0x00010000   /* PINSEL0 bits selecting P0.8 as TXD1 */
#define RxD1_PIN_FUNC 0x00040000   /* PINSEL0 bits selecting P0.9 as RXD1 */

//defines for UXLCR
#define _8BIT 3                   /* Word length select value for 8 data bits */
#define WOR_LEN_SEL_ _8BIT        /* Word length setting used when configuring UxLCR */
#define DLAB_BIT 7                /* Divisor Latch Access Bit position in UxLCR */

//defines for UXLSR
#define DR_BIT 0                  /* Data Ready bit position in UxLSR (rx data available) */
#define TEMT_BIT 6                /* Transmitter Empty bit position in UxLSR (tx complete) */

