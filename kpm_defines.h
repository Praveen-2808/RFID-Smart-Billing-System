/*
 * kpm_defines.h
 * -----------------------------------------------------------------------
 * Pin mapping for the 4x4 matrix keypad (KPM). Rows and columns are all
 * on port 1 (P1), occupying bits 24-31, so a 4-bit "nibble" write can
 * drive all four row lines at once (see WNIBBLE in defines.h).
 * -----------------------------------------------------------------------
 */

/* Row output pins (driven low one at a time while scanning) */
#define ROW0 24//P1.24
#define ROW1 25
#define ROW2 26
#define ROW3 27

/* Column input pins (read to detect which key in the active row is pressed) */
#define COL0 28
#define COL1 29
#define COL2 30
#define COL3 31
