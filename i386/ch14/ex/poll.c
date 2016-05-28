#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/poll.h> 

int main( int argc, char **argv )
{
    int    sfd1, sfd2, sfd3;
    
    struct pollfd Events[3];
    int    retval;
    char   buff[256];
    int    readcnt;

    
    sfd1 = open( "/dev/ttyS1", O_RDWR | O_NOCTTY );
    sfd2 = open( "/dev/ttyS2", O_RDWR | O_NOCTTY );
    sfd3 = open( "/dev/ttyS3", O_RDWR | O_NOCTTY );
    
    
        :
    각각의 시리얼 환경 설정 루틴들
        :
    
    memset( Events, 0, sizeof( Events ) );

    Events[0].fd     =  sfd1;
    Events[0].events = POLLIN          // 수신 이벤트 
                     | POLLERR;        // 에러 이벤트

    Events[1].fd     =  sfd2;
    Events[1].events = POLLIN          // 수신 이벤트 
                     | POLLERR;        // 에러 이벤트
        
    Events[2].fd     =  sfd3;
    Events[2].events = POLLIN          // 수신 이벤트 
                     | POLLERR;        // 에러 이벤트
    
    while(1)
    {
        // 사건이 생기기 전에 대기 한다. 
        retval = poll( (struct pollfd *)&Events, 3, 5000 ); < 0 )
        if( retval < 0 )
        {
            perror("poll error : " );
            exit (EXIT_FAILURE); 
        } 
        if( retval == 0 )
        {
            printf("5 초안에 아무 데이타도 없었다.\n");
            continue;
        }
        
        for (i = 0; i < 3; i++) 
        {
            // 에러에 대한 검사를 수행한다. 
            if( Events[i].revents & POLLERR ) 
            { 
                printf("장치 에러 발생.\n");
                exit (EXIT_FAILURE); 
            }    

            // 수신된 데이타가 있는가를 검사한다.     
            if( Events[i].revents & POLLIN ) 
            { 
                readcnt = read( i, buff, 256 );    
                write( i, buff, readcnt );
            }    
        }    
    }
    
    close( sfd1 );
    close( sfd2 );
    close( sfd3 );
    
}
                    
