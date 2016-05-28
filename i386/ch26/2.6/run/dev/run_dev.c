#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       

#include <asm/uaccess.h>
#include <asm/io.h>


int run_init(void)
{
    int ret;
    char *argv[] = { "/run_app", "first", "second",NULL };
    char *envp[] = { "HOME=/",
                     "TERM=linux",
                     "PATH=/sbin:/usr/sbin:/bin:/usr/bin",
                      NULL };
    
    printk( "run_dev start\n" );
    ret = call_usermodehelper("/run_app", argv, envp, 1);
    
    printk( "result = %d\n", ret );

    return 0;
}

void run_exit(void)
{
   printk( "run_dev end\n" );     
}

module_init(run_init);
module_exit(run_exit);

MODULE_LICENSE("Dual BSD/GPL");

