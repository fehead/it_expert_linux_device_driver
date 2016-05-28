#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILENAME  "/dev/intdev"

typedef struct 
{
    unsigned long time;
} __attribute__ ((packed)) R_INT_INFO;

#define   INT_BUFF_MAX              64

int main()
{
    int        dev;
    R_INT_INFO intbuffer[INT_BUFF_MAX];
    int        intcount;
    char       buff[128];
    int        loop;
    
    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
    if( dev >= 0 )
    {
                 
        printf( "start...\n" );
        buff[0] = 0xFF; 
        write(dev,buff,1 );

        printf( "wait... input\n" );
        while(1)
        {
            memset( intbuffer, 0, sizeof( intbuffer ) );    
            intcount = read(dev,(char *) &intbuffer[0],sizeof(R_INT_INFO) ) / sizeof(R_INT_INFO) ;    
            if( intcount ) break;
        }
        
        printf( "input ok...\n");
        sleep(1);
        memset( intbuffer, 0, sizeof( intbuffer ) );    

        printf( "read interrupt times\n" );
        intcount = read(dev,(char *) intbuffer,sizeof(intbuffer) ) / sizeof(R_INT_INFO) ;    
        for( loop =0; loop < intcount; loop++ )
        {
           printf( "index = %d time = %ld\n", loop, intbuffer[loop].time );
        }   
        
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

