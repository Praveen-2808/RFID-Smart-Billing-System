/*
 * delaydef.c
 * -----------------------------------------------------------------------
 * Implements simple busy-wait (blocking) delay functions declared in
 * delay.h. Each function just counts a "for" loop down to zero; the
 * multiplier values are tuned to approximate the requested time unit
 * for this microcontroller's clock speed. These are not precise/
 * interrupt-safe timers, just rough software delays.
 * -----------------------------------------------------------------------
 */
void delay_us(unsigned int dlyus)
{
/* Scale requested microseconds by 12 loop iterations per microsecond and
 * spin until the counter reaches zero. */
for(dlyus*=12;dlyus>0;dlyus--);
}
void delay_ms(unsigned int dlyms)	 
{
/* Scale requested milliseconds by 12000 loop iterations per millisecond
 * and spin until the counter reaches zero. */
for(dlyms*=12000;dlyms>0;dlyms--);
}
void delay_s(unsigned int dlys)
{
/* Scale requested seconds by 12,000,000 loop iterations per second and
 * spin until the counter reaches zero. */
for(dlys*=12000000;dlys>0;dlys--);
}





