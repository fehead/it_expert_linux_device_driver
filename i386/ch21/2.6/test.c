#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/init.h>

static int onevalue = 1;

MODULE_PARM(onevalue, "i");

static int test_init(void)      
{ 
    printk("Test Kernel Device Driver [onevalue=%d]\n", onevalue );
    return 0; 
}

static void test_exit(void)  
{ 
    printk("Shutdown Kernel Device Driver\n"); 
}

#ifndef MODULE

static int __init test_setup(char *str)
{
	int ints[4];

	str = get_options (str, ARRAY_SIZE(ints), ints);
	if(ints[0] > 0) onevalue = ints[1];
	
	return 1;
}

__setup("test=", test_setup);

__initcall(init_module);

#endif // !MODULE

module_init(test_init);
module_exit(test_exit);
	
MODULE_AUTHOR("You Young-chang frog@falinux.com");
MODULE_DESCRIPTION("Kernel Device Dirver Test Module");
MODULE_LICENSE("Dual BSD/GPL");

	
