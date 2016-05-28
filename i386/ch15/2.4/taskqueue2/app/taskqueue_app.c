#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{

  int           dev;
  char          buff[128];

  printf( "taskqueue led flash start\n" );

  dev = open("/dev/taskqueue", O_RDWR ); 
  if( dev < 0 ) exit(-1);

  printf( "read wait\n" );
  read(dev,buff,1 );

  close(dev);  

  return 0;

}

