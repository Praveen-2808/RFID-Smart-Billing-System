/*
 * uart.h
 * -----------------------------------------------------------------------
 * Public interface for the PC-side (Linux) serial port helper library
 * used by maj_app_pcside.c to talk to the microcontroller board over
 * a USB-serial adapter (e.g. /dev/ttyUSB0). Modeled after WiringPi's
 * serial API.
 * -----------------------------------------------------------------------
 */
#ifndef _UART_H_
#define _UART_H_

int   serialOpen      (const char *device, const int baud) ; /* Open & configure the serial device, returns file descriptor or <0 on error */
void  serialClose     (const int fd) ;                       /* Close the serial device */
void  serialFlush     (const int fd) ;                       /* Flush both tx and rx buffers */
void  serialPutchar   (const int fd, const unsigned char c) ;/* Send a single byte */
int   serialGetchar   (const int fd) ;                       /* Read a single byte (blocking, ~10s timeout) */

#endif
