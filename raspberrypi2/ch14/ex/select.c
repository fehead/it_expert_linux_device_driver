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
    ������ �ø��� ȯ�� ���� ��ƾ��
        :
    
    
    while(1)
    {
        // �б� �̺�Ʈ ����� �Ǵ� �͵� 
        FD_ZERO(&rfds);
        FD_SET(sfd1, &rfds);
        FD_SET(sfd2, &rfds);
        FD_SET(sfd3, &rfds);
        
        // ���� �̺�Ʈ ����� �Ǵ� �͵� 
        FD_ZERO(&errorfds);
        
        FD_SET(sfd1, &errorfds);
        FD_SET(sfd2, &errorfds);
        FD_SET(sfd3, &errorfds);
        
        tv.tv_sec  = 5;   // 5�ʿ� ���� �ð�
        tv.tv_usec = 0;
        
        // ����� ����� ���� ��� �Ѵ�. 
        retval = select(FD_SETSIZE, &rfds, NULL, &errorfds, &tv); 
        if( retval < 0 )
        {
            perror ("select"); 
            exit (EXIT_FAILURE); 
        } 
        if( retval == 0 )
        {
            printf("5 �ʾȿ� �ƹ� ����Ÿ�� ������.\n");
        }
        // ���ŵ� ����Ÿ�� �ִ°��� �˻��Ѵ�. 
        for (i = 0; i < FD_SETSIZE; ++i) 
        {
            if (FD_ISSET (i, &read_fd_set)) 
            { 
                readcnt = read( i, buff, 256 );    
                write( i, buff, readcnt );
            }    
        }    
        // ������ ���� �˻縦 �����Ѵ�. 
        for (i = 0; i < FD_SETSIZE; ++i) 
        {
            if (FD_ISSET (i, &errorfds)) 
            { 
                printf("��ġ ���� �߻�.\n");
                exit (EXIT_FAILURE); 
            }    
        }    
    }
    
    close( sfd1 );
    close( sfd2 );
    close( sfd3 );
    
}
                    
