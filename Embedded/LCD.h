/*
 * LCD.h
 * -----------------------------------------------------------------------
 * Public interface for the character LCD (HD44780-compatible) driver
 * running in 8-bit parallel mode.
 * -----------------------------------------------------------------------
 */
#include "types.h"

void writeLCD(u8 data);           /* Low-level: pulse EN to latch a byte onto the LCD data bus */
void cmdLCD(u8 cmd);              /* Send a command byte (RS=0) to the LCD, e.g. clear, cursor move */
void charLCD(u8 ascii);           /* Send a data byte (RS=1) to the LCD, i.e. print one character */
void InitLCD(void);                /* Power-up / mode initialisation sequence for the LCD */
void strLCD(s8 *p);               /* Print a null-terminated string */
void U32LCD(u32 n);               /* Print an unsigned 32-bit number in decimal */
void s32LCD(s32 n);               /* Print a signed 32-bit number in decimal */
void f32LCD(f32 fnum,u8 nDP);     /* Print a floating point number with nDP decimal places */
void HEXLCD(u32 n);               /* Print a number in hexadecimal */
void BinLCD(u32 n,u8 nbd);        /* Print the lowest nbd bits of n in binary */
void BuildCGRAM(u8* p,u8 nb);     /* Load nb custom character bitmap bytes into LCD CGRAM */

