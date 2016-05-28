#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/kernel.h> 
  
int func_var1 = 0; 
int func_var2 = 0; 
  
int func_sum ( int var3  )  
{  
     printk( "func_var1 = %d\n", func_var1 );  
     printk( "func_var2 = %d\n", func_var2 );  
     printk( "var3      = %d\n", var3 );  
     return func_var1 + func_var2  + var3 ;  
}  
  
int funcdev_init(void)  
{  
    int result;  
      
    return 0;   
}  
  
void funcdev_exit(void)  
{  
          
}  
  
EXPORT_SYMBOL(func_var1);  
EXPORT_SYMBOL(func_var2);  
EXPORT_SYMBOL(func_sum);  
  
module_init(funcdev_init); 
module_exit(funcdev_exit); 
 
MODULE_LICENSE("Dual BSD/GPL"); 
  
