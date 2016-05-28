#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/slab.h>
#include <linux/vmalloc.h>

void kmalloc_test( void )
{
    char *buff;
    
    printk( "kmalloc test\n" );
    
    buff = kmalloc( 1204, GFP_KERNEL );
    if( buff != NULL )
    {
        sprintf( buff, "test memory\n" );
        printk( buff );
    
        kfree( buff );
    }    
    
    buff = kmalloc( 32 * PAGE_SIZE, GFP_KERNEL );
    if( buff != NULL )
    {
        printk( "Big Memory Ok\n" );
        kfree( buff );
    }
    
}

void vmalloc_test( void )
{
    char *buff;

    printk( "vmalloc test\n" );
    
    buff = vmalloc( 33 * PAGE_SIZE );
    if( buff != NULL )
    {
        sprintf( buff, "vmalloc test ok\n" );
        printk( buff );
    
        vfree( buff );
    }    

}

void get_free_pages_test( void )
{
    char *buff;
    int   order;

    printk( "get_free_pages test\n" );
    
    order = get_order(8192*10);
    buff  = __get_free_pages( GFP_KERNEL, order );
    if( buff != NULL) 
    {
        sprintf( buff, "__get_free_pages test ok [%d]\n", order );
        printk( buff );
        
        free_pages(buff, order);
    }
}

int memtest_init(void)      
{ 
    char *data;
    
    printk("Module Memory Test\n" );
    
    kmalloc_test();
    vmalloc_test();
    get_free_pages_test();
    
    return 0; 
}

void memtest_exit(void)  
{ 
    printk("Module Memory Test End\n"); 
}

module_init(memtest_init);
module_exit(memtest_exit);

MODULE_LICENSE("Dual BSD/GPL");
