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
  int           lp;
  char          buff[128];

  printf( "workqueue key start\n" );

  dev = open("/dev/workqueue", O_RDWR ); 
  if( dev < 0 ) exit(-1);

  buff[0] = 0x00; write(dev,buff,1 );

  printf( "read wait\n" );
  for( lp = 0; lp < 3; lp++ )
  {
      read(dev,buff,1 );
      printf( "check input\n" );
  }    
  
  printf( "led flashing\n" );
  for( lp = 0; lp < 3; lp++ )
  {
      buff[0] = 0xFF; write(dev,buff,1 ); sleep(1);
      buff[0] = 0x00; write(dev,buff,1 ); sleep(1);
  }  
  close(dev);  

  return 0;

}

