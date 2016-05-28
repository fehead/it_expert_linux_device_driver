#define MODULE  
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
  
int init_module(void)  
{  
    int result;  
      
    return 0;   
}  
  
void cleanup_module(void)  
{  
          
}  
  
EXPORT_SYMBOL_NOVERS(func_var1);  
EXPORT_SYMBOL_NOVERS(func_var2);  
EXPORT_SYMBOL_NOVERS(func_sum);  
