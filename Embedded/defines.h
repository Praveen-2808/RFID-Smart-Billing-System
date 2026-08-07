#ifndef __DEFINES_H__
#define __DEFINES_H__

/*
 * defines.h
 * -----------------------------------------------------------------------
 * Generic bit/byte/nibble manipulation macros used across the
 * microcontroller-side drivers to read/write individual bits or groups
 * of bits within a register/word without disturbing the other bits.
 * -----------------------------------------------------------------------
 */

/* Set the bit at BITPOS to 1, leaving other bits unchanged */
#define SETBIT(WORD,BITPOS) (WORD|=1<<BITPOS)
/* Clear the bit at BITPOS to 0, leaving other bits unchanged */
#define CLRBIT(WORD,BITPOS) (WORD&=~(1<<BITPOS))
/* Set WORD so that only the bit at BITPOS is 1 (all other bits become 0) */
#define SSETBIT(WORD,BITPOS) (WORD=1<<BITPOS)
/* NOTE: as written this does the same thing as SSETBIT (sets WORD to
 * 1<<BITPOS) rather than clearing it - kept as-is / not modified. */
#define SCLRBIT(WORD,BITPOS) (WORD=1<<BITPOS)
/* Toggle (complement) the bit at BITPOS */
#define CPLBIT(WORD,BITPOS) (WORD^=1<<BITPOS)

/* Write a single BIT (0 or 1) into WORD at BITPOS, leaving other bits unchanged */
#define WBIT(WORD,BITPOS,BIT) (WORD=(WORD&~(1<<BITPOS)|(BIT<<BITPOS)))
/* Write an 8-bit BYTE into WORD starting at bit position SBITPOS,
 * leaving the other bits of WORD unchanged */
#define WBYTE(WORD,SBITPOS,BYTE)\
(WORD=(WORD&~(0XFF<<SBITPOS)|(BYTE<<SBITPOS)))
/* Write a 4-bit NIBBLE into WORD starting at bit position BITPOS,
 * leaving the other bits of WORD unchanged */
#define WNIBBLE(WORD,BITPOS,NIBBLE) (WORD=(WORD&~(0XF<<BITPOS)|(NIBBLE<<BITPOS)))


/* Read the single bit at BITPOS from WORD (returns 0 or 1) */
#define RBIT(WORD,BITPOS) ((WORD>>BITPOS)&1)
/* Read a 4-bit nibble from WORD starting at bit position SBITPOS */
#define RNIBBLE(WORD,SBITPOS) ((WORD>>SBITPOS)&15)
/* Read an 8-bit byte from WORD starting at bit position BITPOS */
#define RBYTE(WORD,BITPOS) ((WORD>>BITPOS)&255)

/* Copy a single bit from bit position SBIT of DWORD itself into bit
 * position DBIT of DWORD (in-place bit copy within the same word) */
#define READWRITEBIT(DWORD,DBIT,SBIT) DWORD=(DWORD&~(1<<DBIT))|(((DWORD>>SBIT)&1)<<DBIT)
/* Copy a single bit from bit position SBIT of SWORD into bit position
 * DBIT of DWORD (bit copy between two different words) */
#define READWRITEBIT2(DWORD,DBIT,SWORD,SBIT) DWORD=(DWORD&~(1<<DBIT))|(((SWORD>>SBIT)&1)<<DBIT)
#endif
