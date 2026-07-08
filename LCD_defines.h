/*
 * LCD_defines.h
 * -----------------------------------------------------------------------
 * Pin mapping and HD44780 command-set constants for the character LCD,
 * running in 8-bit mode with the data bus on P1.16-P1.23.
 * -----------------------------------------------------------------------
 */

#define LCD_DATA 16	//p1.16          /* Starting bit position of the 8-bit LCD data bus on port 1 */
#define RS 4                        /* Register Select pin (0=command, 1=data) */
#define RW 6                        /* Read/Write pin (0=write, 1=read) */
#define EN 5                        /* Enable pin - pulsed high then low to latch data/command */
//LCD_commands
#define CLEAR_LCD 0X01              /* Clear display and return cursor to home */
#define RET_CUR_HOME 0X02           /* Return cursor to home position (line 1, pos 0) */
#define DISP_OFF 0X08               /* Turn display off */
#define DISP_ON 0X0C                /* Turn display on, cursor off */
#define DISP_ON_CUR_ON 0X0E         /* Turn display on, cursor on (no blink) */
#define DISP_ON_CUR_BLINK 0X0F      /* Turn display on, cursor on and blinking */

#define MODE_8BIT_1LINE 0X30        /* Function set: 8-bit interface, 1 display line */
#define MODE_8BIT_2LINE 0X38        /* Function set: 8-bit interface, 2 display lines */
#define MODE_4BIT_1LINE 0X20        /* Function set: 4-bit interface, 1 display line */
#define MODE_4BIT_2LINE 0X28        /* Function set: 4-bit interface, 2 display lines */
#define GOTO_LINE1_POS0 0X80        /* Move cursor to start (col 0) of line 1 (DDRAM addr 0x00) */
#define GOTO_LINE2_POS0 0XC0        /* Move cursor to start (col 0) of line 2 (DDRAM addr 0x40) */
#define GOTO_LINE3_POS0 0X94        /* Move cursor to start (col 0) of line 3 (20x4 displays) */
#define GOTO_LINE4_POS0 0XD4        /* Move cursor to start (col 0) of line 4 (20x4 displays) */
#define SHIFT_CUR_RIGHT 0X06        /* Entry mode: increment cursor, no display shift */
#define SHIFT_DISP_LEFT 0X10        /* Cursor/display shift: shift display left */
#define SHIFT_DISP_RIGHT 0X14       /* Cursor/display shift: shift display right */
#define GOTO_CGRAM 0X40             /* Set CGRAM address (used when loading custom characters) */
