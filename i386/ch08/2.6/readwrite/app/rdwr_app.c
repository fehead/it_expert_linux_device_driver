#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILENAME  "/dev/rdwrdev"

int main()
{
    int dev;
    char buff[128];
    int loop;

    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
    if( dev >= 0 )
    {
                 
        printf( "wait... input\n" );
     
        while(1)
        {
            if( read(dev,buff,1 ) == 1 )
            {
                printf( "read data [%02X]\n", buff[0] & 0xFF );
                if( !(buff[0] & 0x10) ) break;
            }
        } 
        printf( "input ok...\n");
        printf( "led flashing...\n");
        for( loop=0; loop<5; loop++ )
        {
            buff[0] = 0xFF; 
            write(dev,buff,1 );
            sleep(1);
            buff[0] = 0x00; 
            write(dev,buff,1 );
            sleep(1);
        }
        close(dev);
    }

    return 0;
}

