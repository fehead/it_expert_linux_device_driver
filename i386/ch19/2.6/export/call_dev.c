#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

extern int func_var1;
extern int func_var2;
extern int func_sum ( int sub3  );

int calldev_init(void)
{
    int result;
    
    func_var1 = 3;
    func_var2 = 4;
    
    printk( "%d + %d + 5 =  %d\n", func_var1, func_var2, func_sum(5) );
    
    return 0; 
}

void calldev_exit(void)
{
        
}

module_init(calldev_init);
module_exit(calldev_exit);

MODULE_LICENSE("Dual BSD/GPL");
