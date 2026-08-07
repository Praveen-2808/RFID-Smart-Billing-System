/*
 * LCD.c
 * -----------------------------------------------------------------------
 * Driver for an HD44780-compatible character LCD running in 8-bit
 * parallel mode. Data lines are on P1.16-P1.23; RS/RW/EN control lines
 * are on P0 (see LCD_defines.h). Provides low-level command/character
 * writes plus helpers to print unsigned/signed integers, floats, hex,
 * binary, and to load custom characters into CGRAM.
 * -----------------------------------------------------------------------
 */
#include <lpc21xx.h>
#include "LCD_defines.h"
#include "defines.h"
#include "delay.h"
#include "types.h"
/* Low-level byte write: places `data` on the 8-bit data bus and pulses
 * the Enable (EN) line to latch it into the LCD (as either a command or
 * character, depending on what RS was previously set to). */
void writeLCD(u8 data)
{
	SCLRBIT(IOCLR0,RW);//RW=0 write operation
	WBYTE(IOPIN1,LCD_DATA,data);
	SSETBIT(IOSET0,EN);//EN=1
	delay_us(1);
	SCLRBIT(IOCLR0,EN);//EN=0
	delay_ms(2);//internal process 
}
/* Sends a command byte to the LCD (RS=0 selects the instruction register). */
void cmdLCD(u8 cmd)
{
	SCLRBIT(IOCLR0,RS);//RS=0,command reg selected
	writeLCD(cmd);
}
/* Sends a character/data byte to the LCD (RS=1 selects the data register). */
void charLCD(u8 ascii)
{
	SSETBIT(IOSET0,RS);//RS=1,data reg selected
	writeLCD(ascii);
}
/* Power-up initialisation sequence for the LCD: configures the data
 * pins and control pins as outputs, then issues the standard HD44780
 * "function set" sequence (repeated per datasheet timing requirements)
 * before switching to 2-line mode, turning the display on with a
 * blinking cursor, and clearing the display. */
void InitLCD()
{
	WBYTE(IODIR1,LCD_DATA,255);
	SETBIT(IODIR0,RS);//p0.16 as output
	SETBIT(IODIR0,RW);//p0.17 as output
	SETBIT(IODIR0,EN);//p0.18 as output
	
	delay_ms(15);
	cmdLCD(MODE_8BIT_1LINE);
	delay_ms(5);
	cmdLCD(MODE_8BIT_1LINE);
	delay_us(100);
	cmdLCD(MODE_8BIT_1LINE);
	cmdLCD(MODE_8BIT_2LINE);
	cmdLCD(DISP_ON_CUR_BLINK);
	cmdLCD(0x01);
	//cmdLCD(0x10);
	//cmdLCD(0x06);
}
/* Prints each character of a null-terminated string in sequence. */
void strLCD(s8 *p)
{
	while(*p)
	{
		charLCD(*p);
		p++;
	}
}
/* Prints an unsigned 32-bit integer in decimal. Extracts digits
 * least-significant-first into a temporary buffer, then prints them in
 * reverse (most-significant-first) order. */
void U32LCD(u32 n)
{
	u8 a[10];
	s32 i=0;
	if(n==0)
	{
		charLCD('0');
	}
	else 
	{
		while(n)
		{
			a[i++]=(n%10)+48;
			n/=10;
		}
		for(--i;i>=0;i--)
		charLCD(a[i]);
	}
}
/* Prints a signed 32-bit integer: emits a '-' for negative values, then
 * prints the absolute value via U32LCD. */
void s32LCD(s32 n)
{
	if(n<0)
	{
		charLCD('-');
		n=-n;
	}	
		U32LCD(n);

}
/* Prints a floating point number with a fixed number of decimal places
 * (nDP). Prints the integer part via U32LCD, then repeatedly multiplies
 * the fractional remainder by 10 to extract each subsequent digit. */
void f32LCD(f32 fnum,u8 nDP)
{
	u32 n;
	s32 i;
	if(fnum<0)
	{
		charLCD('-');
	}
	n=fnum;
	U32LCD(n);
	charLCD('.');

	for(i=0;i<nDP;i++)
	{
		fnum=(fnum-n)*10;
		n=fnum;
		charLCD(n+48);
	}
}
/* Prints an unsigned 32-bit integer in hexadecimal (uppercase digits
 * A-F). Extracts hex digits least-significant-first, then prints them
 * in reverse order. */
void HEXLCD(u32 n)
{
	u8 a[8],rem;
	s32 i=0;
	if(n==0)
	{
		charLCD('0');
	}
	else
	{
		while(n)
		{
			rem=n%16;
			(rem<10)?(rem+=48):(rem+=55);
			a[i++]=rem;
			n/=16;
		}
		for(--i;i>=0;i--)
		{
			charLCD(a[i]);
		}
	}
}
/* Prints the lowest nbd bits of n as a binary string, most-significant
 * bit first. */
void BinLCD(u32 n,u8 nbd)
{
	s32 i;
	for(i=(nbd-1);i>=0;i--)
	{
		charLCD(((n>>i)&1)+48);
	}
}
/* Loads nb custom character pattern bytes (p) into the LCD's CGRAM
 * starting at address 0, then returns the cursor to line 1, position 0
 * so subsequent output does not overwrite the custom character data. */
void BuildCGRAM(u8* p,u8 nb)
{
	s32 i;
	cmdLCD(GOTO_CGRAM);
	for(i=0;i<=nb;i++)
	{
		charLCD(p[i]);
	}
	cmdLCD(GOTO_LINE1_POS0);
}

