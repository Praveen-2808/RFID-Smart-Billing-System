/*
 * interrupt.h
 * -----------------------------------------------------------------------
 * Declarations for the ARM7 (LPC21xx) interrupt service routines (ISRs)
 * used by this project. Each is marked with the Keil/ARM-specific
 * "__irq" keyword so the compiler generates the correct entry/exit code
 * (register save/restore) for an interrupt handler.
 * -----------------------------------------------------------------------
 */

void uart1_isr(void)__irq;   /* Handles incoming bytes on UART1 (RFID/keypad link) */
void uart0_isr(void)__irq;   /* Handles incoming bytes on UART0 (PC-side link) */
void eint0_isr(void)__irq;   /* External interrupt 0 - "Entry" push button */
void eint1_isr(void)__irq;   /* External interrupt 1 - "Delete" push button */
void eint2_isr(void)__irq;   /* External interrupt 2 - "Exit/Checkout" push button */
