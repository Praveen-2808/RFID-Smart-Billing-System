/*
 * uart.c
 * -----------------------------------------------------------------------
 * PC-side (Linux) serial port helper library, based on the WiringPi
 * serial API. Used by maj_app_pcside.c to open, read from, and write to
 * a USB-serial device connected to the microcontroller board.
 * -----------------------------------------------------------------------
 */
#include <stdint.h>
#include <termios.h>
#include <fcntl.h>
#include<unistd.h>


/* serialOpen: Open and initialise the serial port, 
   setting all the right port parameters - or 
   as many as are required - hopefully! */

int serialOpen (const char *device, const int baud)
{
  struct termios options ;
  speed_t myBaud ;
  int     status, fd ;

  /* Translate the requested integer baud rate into the termios speed_t
   * constant required by cfsetispeed/cfsetospeed. Unsupported rates
   * fall through to the default case below. */
  switch (baud)
  {
    case     50:	myBaud =     B50 ; break ;
    case     75:	myBaud =     B75 ; break ;
    case    110:	myBaud =    B110 ; break ;
    case    134:	myBaud =    B134 ; break ;
    case    150:	myBaud =    B150 ; break ;
    case    200:	myBaud =    B200 ; break ;
    case    300:	myBaud =    B300 ; break ;
    case    600:	myBaud =    B600 ; break ;
    case   1200:	myBaud =   B1200 ; break ;
    case   1800:	myBaud =   B1800 ; break ;
    case   2400:	myBaud =   B2400 ; break ;
    case   4800:	myBaud =   B4800 ; break ;
    case   9600:	myBaud =   B9600 ; break ;
    case  19200:	myBaud =  B19200 ; break ;
    case  38400:	myBaud =  B38400 ; break ;
    case  57600:	myBaud =  B57600 ; break ;
    case 115200:	myBaud = B115200 ; break ;
    case 230400:	myBaud = B230400 ; break ;

    default:
      /* Unsupported baud rate requested. */
      return -2 ;
  }

  /* Open the device for read/write, without making it the controlling
   * terminal for this process. */
  if ((fd = open (device, O_RDWR | O_NOCTTY )) == -1)
    return -1 ;

  // Get and modify current options:

  tcgetattr (fd, &options) ;

    /* Put the port into "raw" mode (no line editing/echo/signal
     * processing) and set the requested baud rate for both input and
     * output. */
    cfmakeraw   (&options) ;
    cfsetispeed (&options, myBaud) ;
    cfsetospeed (&options, myBaud) ;

    /* Enable the receiver and set local mode (ignore modem control
     * lines); disable parity and 2-stop-bit mode; set 8 data bits. */
    options.c_cflag |= (CLOCAL | CREAD) ;
    options.c_cflag &= ~PARENB ;
    options.c_cflag &= ~CSTOPB ;
    options.c_cflag &= ~CSIZE ;
    options.c_cflag |= CS8 ;
    /* Disable canonical (line-buffered) mode, echo, and signal
     * generation so raw bytes are read exactly as sent. */
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG) ;
    /* Disable output processing so bytes are written exactly as given. */
    options.c_oflag &= ~OPOST ;

  /* Apply the new settings immediately, discarding any pending I/O. */
  tcsetattr (fd, TCSANOW | TCSAFLUSH, &options) ;

  usleep (10000) ;	// 10mS

  return fd ;
}


/* serialFlush: Flush the serial buffers (both tx & rx) */

void serialFlush (const int fd)
{
  tcflush (fd, TCIOFLUSH) ;
}


/* serialClose: Release the serial port */

void serialClose (const int fd)
{
  close (fd) ;
}


/* serialPutchar: Send a single character to the serial port */

void serialPutchar (const int fd, const unsigned char c)
{
  write (fd, &c, 1) ;
}



/** serialGetchar: Get a single character from the serial device.
 * Note: Zero is a valid character and this function will time-out after 10 seconds.
 **********************************************************************************/

int serialGetchar (const int fd)
{
  uint8_t x ;

  /* Blocking read of exactly one byte; returns -1 if the read failed or
   * timed out. */
  if (read (fd, &x, 1) != 1)
    return -1 ;

  return ((int)x) & 0xFF ;
}
