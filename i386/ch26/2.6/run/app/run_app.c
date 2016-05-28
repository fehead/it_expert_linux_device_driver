#include <stdio.h>
#include <syslog.h>

int main( int argc, char **argv )
{
    int lp;
    
    syslog(LOG_INFO|LOG_LOCAL0, "start run_app\n" );    
    syslog(LOG_INFO|LOG_LOCAL0, "argc = %d\n", argc );
    for( lp = 0; lp < argc; lp++ )
    {
        syslog(LOG_INFO|LOG_LOCAL0, "argv[%d] = [%s]\n", lp, argv[lp] );
    }
    
    sleep(3);
    
    return 0;
}

