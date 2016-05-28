#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/poll.h> 

#define   ARMDIO_IOCTL_MAGIC    'a'
#define   ARMDIO_IOCTL_SET_MODE _IOW( ARMDIO_IOCTL_MAGIC, 0 , unsigned long )
#define   ARMDIO_IOCTL_SET_TIME _IOW( ARMDIO_IOCTL_MAGIC, 1 , unsigned long )

unsigned char bit_pattern[] 
  = { 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01,0x02,0x04,0x08,0x10,0x20,0x40 };

int main()
{
    int           dev;
    struct pollfd Events[1];
    int           retval;
    char          buff[128];
    char          buff2[128];
    int           readcnt; 
    int           loop;
    int           keystate;
    char          key;
    int           outmode;
    
    printf( "arm dio program start\n" );

    dev = open("/dev/armdio", O_RDWR ); 
    if( dev < 0 )
    {
        printf( "[/dev/armdio] Open fail\n"); 
        exit(-1);
    }

    outmode = 0;
    ioctl(dev, ARMDIO_IOCTL_SET_TIME, 1000 );

    while(1)
    {
        memset( Events, 0, sizeof( Events ) );
        Events[0].fd     =  dev;
        Events[0].events = POLLIN | POLLOUT; 

        retval = poll( (struct pollfd *)&Events, 1, 10000 );
        
        if( retval < 0 )
        {
            perror("poll error : " );
            exit (EXIT_FAILURE); 
        } 
        
        if( retval == 0 ) continue;
        
        if( Events[0].revents & POLLERR ) 
        { 
            printf("Device Error\n");
            exit(EXIT_FAILURE); 
        }
        
        if( Events[0].revents & POLLIN ) 
        { 
            readcnt = read( dev, buff, sizeof( buff ) );    
            for( loop = 0; loop < readcnt; loop++ )
            {
                key = buff[loop];
                if( key & 0x80 ) printf( "KEY UP   -> ");
                else             printf( "KEY DOWN -> ");
                
                switch( key )
                {
                case '0' : outmode = 1;
                           ioctl(dev, ARMDIO_IOCTL_SET_MODE, outmode ); 
                           write( dev, bit_pattern, sizeof(bit_pattern) );
                           break;
                case '1'...'7' :            
                           outmode = 0;
                           ioctl(dev, ARMDIO_IOCTL_SET_MODE, outmode ); 
                           buff2[0] = 1 << ( key & 0xF );
                           write( dev, buff2, 1 );
                           break;
                }

                key &= 0x7F;   
                printf( "[%c]\n", key );
            }
        }
        if( Events[0].revents & POLLOUT ) 
        { 
            if( outmode ) write( dev, bit_pattern, sizeof(bit_pattern) );
        }        
    }
    
    close(dev);  

    return 0;
}

