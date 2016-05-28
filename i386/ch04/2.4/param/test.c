#define MODULE
#include <linux/module.h>
#include <linux/kernel.h>

static int onevalue = 1;
static char *twostring = NULL;

MODULE_PARM(onevalue, "i");
MODULE_PARM(twostring, "s");

int init_module(void)      
{ 
    printk("Hello, world [onevalue=%d:twostring=%s]\n", onevalue, twostring );
    return 0; 
}

void cleanup_module(void)  
{ 
    printk("Goodbye world\n"); 
}

MODULE_AUTHOR("You Young-chang frog@falinux.com");
MODULE_DESCRIPTION("Module Parameter Test Module");

