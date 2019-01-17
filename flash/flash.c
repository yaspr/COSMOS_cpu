#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_STR 80

typedef unsigned char byte;

//
int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */
    
    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

//
void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

//
int main(int argc, char **argv)
{
  if (argc < 2)
    return printf("OUPS! %s [serial port]\n", argv[0]), -1;
  
  unsigned char buf[MAX_STR + 1];
  unsigned char _code_[2048];
  int len;
  int fd;
  unsigned _4bytes_ = 0;
  unsigned char _byte_ = 0;
  FILE *ifd = NULL;
  unsigned char _is_command_ = 0;
  
  fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);

  if (fd < 0)
    {
      printf("Error opening %s: %s\n", argv[1], strerror(errno));
      return -1;
    }
  
  //Set serial to 9600bps (channel bandwidth)
  set_interface_attribs(fd, B9600); 
  set_mincount(fd, 0);


  printf("Press enter to start!\n");

  //Interactive mode
  //Commands: send, exit
  do
    {
      printf("# ");
      fgets(buf, MAX_STR, stdin);
      
      if (!strncmp(buf, "send", 4))
	{
	  _is_command_ = 1;
	  
	  do
	    {
	      printf("   file path: ");
	      fgets(buf, MAX_STR, stdin);
	      
	      buf[strlen(buf) - 1] = 0;
	      
	      ifd = fopen(buf, "rb");
	      
	      if (!ifd)
		printf("#Error# Cannot find file: %s\n", buf);
	    }
	  while (!ifd);

	  //Send file 4 bytes at a time 
	  while (fread(&_4bytes_, sizeof(unsigned), 1, ifd))
	    {	      
	      buf[0] = _4bytes_ >> 24;
	      buf[1] = _4bytes_ >> 16;
	      buf[2] = _4bytes_ >>  8;
	      buf[3] = _4bytes_;

	      write(fd, buf, 4);
	    }

	  //Send execute command (4 bytes, _EXE or 0x5F455845
	  buf[0] = '_';
	  buf[1] = 'E';
	  buf[2] = 'X';
	  buf[3] = 'E';
	  
	  write(fd, buf, 4);
	}
      else
	if (!strncmp(buf, "ping", 4))
	  {
	    _is_command_ = 0;

	    buf[0] = '_';
	    buf[1] = 'P';
	    buf[2] = 'N';
	    buf[3] = 'G';

	    write(fd, buf, 4);
	  }
	else
	  if (!strncmp(buf, "exit", 4))
	    printf("Bye bye!\n"), exit(0);
	  else
	    if (!strncmp(buf, "blink", 5))
	      {
		_is_command_ = 0;

		//Send blink command
		buf[0] = '_';
		buf[1] = 'B';
		buf[2] = 'L';
		buf[3] = 'N';

		write(fd, buf, 4);
	      }
	    else
	      _is_command_ = 0;
      
      //
      if (_is_command_)
	printf("   output:\n");
      
      //Reading board's output ...
      //Serial write adds a \r and \n characters at the end of a buffer
      while (read(fd, &_byte_, 1))
	printf("%c", _byte_);

    }
  while (1);
  
  return 0;
}
