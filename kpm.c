/*
 * kpm.c
 * -----------------------------------------------------------------------
 * Driver for a 4x4 matrix keypad, wired with rows and columns on
 * P1.24-P1.31 (see kpm_defines.h). Scanning works by driving one row
 * low at a time and checking which column line reads low, which
 * identifies the pressed key via the kpmLUT lookup table.
 * Also provides higher-level helpers (Readnum/ReadPIN) that read a
 * full number/PIN from the keypad while echoing to the LCD.
 * -----------------------------------------------------------------------
 */
#include "types.h"																	       
#include "defines.h"
#include <lpc21xx.h>
#include "kpm_defines.h"
#include "lcd.h"
#include"delay.h"

//u32 kpmLUT[4][4]={{1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16}};

/* Lookup table mapping [row][col] to the character printed on that key
 * of the keypad. */
u8 kpmLUT[4][4]={{'1','2','3','A'},
				 {'4','5','6','B'},
				 {'7','8','9','C'},
				 {'*','0','#','D'}}; 
/* Configure the four row pins (P1.24-P1.27) as outputs; column pins
 * remain inputs. */
void Initkpm()
{
	WNIBBLE(IODIR1,ROW0,15);
}
/* Reads the four column input pins as a group. Returns 0 if at least
 * one column line is pulled low (a key in the currently-driven row is
 * pressed), or 1 if all columns read high (no key pressed in this row). */
u32 colscan()
{
	return (RNIBBLE(IOPIN1,COL0)<15?0:1);
}
/* Scans through each row, driving it low (and the others high) one at a
 * time, and checks colscan() to detect which row currently has a key
 * pressed. Returns the row index (0-3), then restores all rows to the
 * default (all low / not driven high) state before returning. */
u32 Rowcheck()
{
	u32 rno;
	for(rno=0;rno<4;rno++)
	{
		/* Drive only row `rno` low (bit cleared), all other rows high. */
		WNIBBLE(IOPIN1,ROW0,~(1<<rno));
		if(colscan()==0)
		{
			break;
		}
	}
		//make rows as default
		WNIBBLE(IOPIN1,ROW0,0X0);
		return rno;
}
/* Checks each column input pin individually to find which one reads
 * low (pressed) for the currently active row. Returns the column
 * index (0-3). */
u32 Colcheck()
{
	u32 cno;
	for(cno=0;cno<4;cno++)
	{
		if(RBIT(IOPIN1,(COL0+cno))==0)
			{
				break;
			}
	}
	return cno;
}
/* Blocks until a key is pressed, determines its row/column, looks up
 * the corresponding character in kpmLUT, waits for the key to be
 * released, then returns the character. */
u8 keyscan()
{
	u8 keyv;
	u32 rno,cno;
	//waits for switch press
	while(colscan());
	//find the rno
	rno=Rowcheck();
	//find the cno
	cno=Colcheck();
	//get the key value using KPMLUT
	keyv=kpmLUT[rno][cno];
	//wait for switch released
	while(!colscan());
	return keyv;
}
/* Reads a sequence of digit keys from the keypad, echoing each digit to
 * the LCD (2nd line) and accumulating them into a decimal number.
 * 'C' acts as a backspace (divides the running total by 10, removing
 * the last digit and updating the LCD accordingly); '#' terminates
 * entry and returns the accumulated value. */
u32 Readnum()
{
	u8 key;
	u32 sum=0;
	cmdLCD(0xc0);
	while(1)
	{
		key=keyscan();
		delay_ms(100);

		if(key>='0'&&key<='9')
		{
			charLCD(key);
			sum=(sum*10)+key-'0';
		}
		else if(key=='#')
		{
			break;
		}
		else if(key=='C')
		{
			/*cmdLCD(0xc0);
			strLCD("          ");
			cmdLCD(0xc0);
			sum=0;*/
			/* Backspace: drop the last digit from the running total. */
			sum/=10;
			/* Clear the number field on line 2 and redraw the updated value. */
			cmdLCD(0xc0);
			strLCD("          ");
			cmdLCD(0xc0);
			U32LCD(sum);
			if(sum==0)
			{
				/* All digits removed: clear the field entirely. */
				cmdLCD(0xc0);
				strLCD("          ");
				cmdLCD(0xc0);
			}

		}
	}
	return sum;
}
/* Reads a PIN from the keypad (max 4 digits), masking each entered
 * digit on the LCD as '*'. 'C' acts as a backspace (only while at
 * least one digit has been entered) and erases the last '*' shown on
 * the LCD. '#' terminates entry and returns the accumulated PIN value
 * immediately. */
u32 ReadPIN()
{
	u8 key;
	u32 sum=0;
	s32 i=0;
	cmdLCD(0xc0);
	while(1)
	{
		key=keyscan();
		delay_ms(100);

		if((key>='0'&&key<='9')&&i!=4)
		{
			/* Show a mask character instead of the actual digit. */
			charLCD('*');
			sum=(sum*10)+key-'0';
			++i;
		}
		else if((key=='C')&&i!=0)
		{
			/* Backspace: drop the last digit and erase its '*' on the LCD. */
			sum/=10;
			--i;
			cmdLCD(0xc0+(i));
			charLCD(' ');
			cmdLCD(0xc0+(i));
		}
		else if((key=='#'))
		{
			return sum;
			//break;
		}
	}
//	return sum;
}
