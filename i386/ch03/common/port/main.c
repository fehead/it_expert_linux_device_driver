#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main( int argc, char **argv )
{
    int fd;
    int lp;
    
    unsigned char  buff[128];
    
    fd = open( "/dev/port", O_RDWR );   
    if( fd < 0 ) 
    {
        perror( "/dev/port open error" );
        exit(1);
    }    
    
    for( lp = 0; lp < 10; lp++ )
    {
        lseek( fd, 0x378, SEEK_SET );
        buff[0] = 0xFF;
        write( fd, buff, 1 );
        sleep( 1 );
        lseek( fd, 0x378, SEEK_SET );
        buff[0] = 0x00;
        write( fd, buff, 1 );
        sleep( 1 );
    }
    close( fd );
    
    return 0;
}

