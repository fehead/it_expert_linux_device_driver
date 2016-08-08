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
    ������ �ø��� ȯ�� ���� ��ƾ��
        :
    
    memset( Events, 0, sizeof( Events ) );

    Events[0].fd     =  sfd1;
    Events[0].events = POLLIN          // ���� �̺�Ʈ 
                     | POLLERR;        // ���� �̺�Ʈ

    Events[1].fd     =  sfd2;
    Events[1].events = POLLIN          // ���� �̺�Ʈ 
                     | POLLERR;        // ���� �̺�Ʈ
        
    Events[2].fd     =  sfd3;
    Events[2].events = POLLIN          // ���� �̺�Ʈ 
                     | POLLERR;        // ���� �̺�Ʈ
    
    while(1)
    {
        // ����� ����� ���� ��� �Ѵ�. 
        retval = poll( (struct pollfd *)&Events, 3, 5000 ); < 0 )
        if( retval < 0 )
        {
            perror("poll error : " );
            exit (EXIT_FAILURE); 
        } 
        if( retval == 0 )
        {
            printf("5 �ʾȿ� �ƹ� ����Ÿ�� ������.\n");
            continue;
        }
        
        for (i = 0; i < 3; i++) 
        {
            // ������ ���� �˻縦 �����Ѵ�. 
            if( Events[i].revents & POLLERR ) 
            { 
                printf("��ġ ���� �߻�.\n");
                exit (EXIT_FAILURE); 
            }    

            // ���ŵ� ����Ÿ�� �ִ°��� �˻��Ѵ�.     
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
                    
