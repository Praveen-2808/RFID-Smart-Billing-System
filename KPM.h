/*
 * KPM.h
 * -----------------------------------------------------------------------
 * Public interface for the 4x4 matrix keypad driver.
 * -----------------------------------------------------------------------
 */
#include "types.h"

void Initkpm(void);     /* Configure row pins as outputs */
u32 colscan(void);      /* Returns 0 if any column line is pulled low (key pressed), else 1 */
u32 Rowcheck(void);     /* Scans rows one at a time, returns index (0-3) of the pressed row */
u32 Colcheck(void);     /* Returns index (0-3) of the pressed column */
u32 keyscan(void);      /* Blocks until a key is pressed and released, returns its ASCII value */
u32 Readnum(void);      /* Reads a multi-digit number from the keypad, shown on the LCD, terminated by '#' */
u32 ReadPIN(void);      /* Reads a 4-digit PIN from the keypad, shown as '*' on the LCD, terminated by '#' */
/*u8 password_1(void);
u8 password_2(char *);*/
