/*
 * major_proj.h
 * -----------------------------------------------------------------------
 * Top-level project header for the microcontroller-side application.
 * Declares the master hardware initialisation routine used by main().
 * -----------------------------------------------------------------------
 */

/* Initialises all on-chip peripherals used by the project: GPIO, LCD,
 * keypad, UART0/UART1 and the external interrupts. Must be called once
 * at the start of main() before any other peripheral is used. */
void init(void);
