#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main( int argc, char **argv )
{
    int    sfd1, sfd2, sfd3;
    
    fd_set rfds;
    fd_set errorfds;
    struct timeval tv;
    int    retval;
    char   buff[256];
    int    readcnt;

    
    sfd1 = open( "/dev/ttyS1", O_RDWR | O_NOCTTY );
    sfd2 = open( "/dev/ttyS2", O_RDWR | O_NOCTTY );
    sfd3 = open( "/dev/ttyS3", O_RDWR | O_NOCTTY );
    
    
        :
    각각의 시리얼 환경 설정 루틴들
        :
    
    
    while(1)
    {
        // 읽기 이벤트 대상이 되는 것들 
        FD_ZERO(&rfds);
        FD_SET(sfd1, &rfds);
        FD_SET(sfd2, &rfds);
        FD_SET(sfd3, &rfds);
        
        // 에러 이벤트 대상이 되는 것들 
        FD_ZERO(&errorfds);
        
        FD_SET(sfd1, &errorfds);
        FD_SET(sfd2, &errorfds);
        FD_SET(sfd3, &errorfds);
        
        tv.tv_sec  = 5;   // 5초에 대한 시간
        tv.tv_usec = 0;
        
        // 사건이 생기기 전에 대기 한다. 
        retval = select(FD_SETSIZE, &rfds, NULL, &errorfds, &tv); 
        if( retval < 0 )
        {
            perror ("select"); 
            exit (EXIT_FAILURE); 
        } 
        if( retval == 0 )
        {
            printf("5 초안에 아무 데이타도 없었다.\n");
        }
        // 수신된 데이타가 있는가를 검사한다. 
        for (i = 0; i < FD_SETSIZE; ++i) 
        {
            if (FD_ISSET (i, &read_fd_set)) 
            { 
                readcnt = read( i, buff, 256 );    
                write( i, buff, readcnt );
            }    
        }    
        // 에러에 대한 검사를 수행한다. 
        for (i = 0; i < FD_SETSIZE; ++i) 
        {
            if (FD_ISSET (i, &errorfds)) 
            { 
                printf("장치 에러 발생.\n");
                exit (EXIT_FAILURE); 
            }    
        }    
    }
    
    close( sfd1 );
    close( sfd2 );
    close( sfd3 );
    
}
                    
