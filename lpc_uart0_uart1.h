/*
 * uart0_uart1.h
 * -----------------------------------------------------------------------
 * Public interface for the on-chip UART0 and UART1 drivers (microcontroller
 * side). UART0 talks to the PC-side application; UART1 talks to the
 * RFID/keypad card-reader module.
 * -----------------------------------------------------------------------
 */
#include"types.h"

void Init_UART0(void);          /* Configure UART0: pins, baud rate, interrupt */
void U0_TxByte(u8 byte);        /* Blocking transmit of a single byte on UART0 */
void U0_Txstring(s8 *p);        /* Blocking transmit of a null-terminated string on UART0 */
void Init_UART1(void);          /* Configure UART1: pins, baud rate, interrupt */
void U1_TxByte(u8 byte);        /* Blocking transmit of a single byte on UART1 */
void U1_Txstring(s8 *p);        /* Blocking transmit of a null-terminated string on UART1 */
void U0_Txint(s32 num);         /* Transmit an integer (as text) on UART0 */
