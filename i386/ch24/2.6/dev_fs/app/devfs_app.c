#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define DEVICE_FILENAME  "/dev/mmapcall"

#define MMAP_SIZE   0x1000

int main()
{
    int        dev;
    int        loop;
    char       *ptrdata;
    
    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
    
    if( dev >= 0 )
    {
       ptrdata = (char *) mmap( 0x12345000, 
                                MMAP_SIZE, 
                                PROT_READ|PROT_WRITE,
                                MAP_SHARED,
                                dev,
                                0x87654000 );
       
       munmap( ptrdata, MMAP_SIZE );

       close(dev);
    }

    return 0;
}

