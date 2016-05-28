#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "ioctl_test.h"

#define DEVICE_FILENAME  "/dev/ioctldev"

int main()
{
    ioctl_test_info  info;
    int              dev;
    int              state;
    int              cnt;
    
    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
    if( dev >= 0 )
    {
                 
        printf( "wait... input\n" );
        ioctl(dev, IOCTLTEST_LEDON ); 
        while(1)
        {
            state = ioctl(dev, IOCTLTEST_GETSTATE );
            if( !(state & 0x10) ) break;
        } 
        
        sleep(1);
        ioctl(dev, IOCTLTEST_LEDOFF ); 
        
        printf( "wait... input\n" );
        while(1)
        {
            info.size = 0;
            ioctl(dev, IOCTLTEST_READ, &info );
            if( info.size > 0 ) 
            {
                if( !(info.buff[0] & 0x10) ) break;
            }
            
        }    
        info.size = 1;
        info.buff[0] = 0xFF;
        for( cnt=0; cnt<10; cnt++ )
        {
            ioctl(dev, IOCTLTEST_WRITE, &info );
            
            info.buff[0] = ~info.buff[0];
            usleep( 500000 );
        }
        
        printf( "wait... input\n" );
        cnt   = 0;
        state = 0xFF;
        
        while(1)
        {
            info.size    = 1;
            info.buff[0] = state;
            ioctl(dev, IOCTLTEST_WRITE_READ, &info );
            
            if( info.size > 0 ) 
            {
                if( !(info.buff[0] & 0x10) ) break;
            }
            
            cnt++;
            if( cnt >= 2 )
            { 
                cnt = 0; 
                state = ~state;
            }     
            usleep( 100000 );
        }
        ioctl(dev, IOCTLTEST_LEDOFF ); 

        close(dev);
    }

    return 0;
}

