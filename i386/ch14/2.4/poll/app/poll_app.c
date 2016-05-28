#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/poll.h> 

int main()
{

    int           dev;
    struct pollfd Events[1];
    int           retval;
  
    char          buff[128];
    int           readcnt; 
    int           loop;
    int           flashing;

    printf( "poll Program Start\n" );
    dev = open("/dev/polldev", O_RDWR ); 
    if( dev < 0 )
    {
        printf( "[/dev/polldev] Open fail\n"); 
        exit(-1);
    }

    flashing = 0;
    printf( "wait poll \n" );
    buff[0] = 0xff;  write(dev,buff,1 ); 
    while(1)
    {
        memset( Events, 0, sizeof( Events ) );
        Events[0].fd     =  dev;
        Events[0].events = POLLIN; 

        retval = poll( (struct pollfd *)&Events, 1, 1000 );
        
        if( retval < 0 )
        {
            perror("poll error : " );
            exit (EXIT_FAILURE); 
        } 
        
        if( retval == 0 )
        {
            flashing = !flashing;
            if( flashing ) buff[0] = 0x00; 
            else           buff[0] = 0xff;
            
            write(dev,buff,1 );
            continue;
        }
        
        if( Events[0].revents & POLLERR ) 
        { 
            printf("Device Error\n");
            exit(EXIT_FAILURE); 
        }
        
        if( Events[0].revents & POLLIN ) 
        { 
            readcnt = read( dev, buff, sizeof( buff ) );    
            printf( "READ DATA COUNT [%d]\n", readcnt );
            for( loop = 0; loop < readcnt; loop++ )
            {
                printf( "READ DATA [%02X]\n", buff[loop] );
            }
        }
    }

    buff[0] = 0x00;  write(dev,buff,1 ); 
  
    close(dev);  

    return 0;
}

