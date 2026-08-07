/*
 * delay.h
 * -----------------------------------------------------------------------
 * Simple busy-wait (software) delay routines used to add fixed pauses
 * in microsecond, millisecond and second units. These are blocking
 * delays implemented as CPU-cycle-burning loops (see delaydef.c).
 * -----------------------------------------------------------------------
 */

void delay_us(unsigned int dlyus);   /* busy-wait delay in microseconds */
void delay_ms(unsigned int dlyms);   /* busy-wait delay in milliseconds */
void delay_s(unsigned int dlys);     /* busy-wait delay in seconds */
