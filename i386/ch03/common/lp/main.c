#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/lp.h>

int main( int argc, char **argv )
{

    int fd;
    int prnstate;   
    int lp;
    
    unsigned char  buff[128];
    
    fd = open( "/dev/lp0", O_RDWR | O_NDELAY );
    if( fd < 0 ) 
    {
        perror( "open error" );
        exit(1);
    }    
  
    while( 1 )
    {  
        ioctl( fd, LPGETSTATUS, &prnstate );
        
        // 13 Pin <--> GND Pin
        if( prnstate & LP_PSELECD ) printf( "ON\n" );
        else                        printf( "OFF\n" ); 
        usleep( 50000 );
    }
    
    close( fd );
    
    return 0;

}

