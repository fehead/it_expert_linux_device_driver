#define MODULE  
#include <linux/module.h>  
#include <linux/kernel.h>        
  
#include <linux/fs.h>            
  
#define   CALL_DEV_NAME            "calldev"  
#define   CALL_DEV_MAJOR            240        
  
int call_open (struct inode *inode, struct file *filp)  
{  
    int num = MINOR(inode->i_rdev);  
  
    printk( "call open -> minor : %d\n", num );  
    MOD_INC_USE_COUNT;  
      
    return 0;  
}  
  
loff_t call_llseek (struct file *filp, loff_t off, int whence )  
{  
    printk( "call llseek -> off : %08X, whenec : %08X\n", off, whence );  
    return 0x23;  
}  
  
ssize_t call_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)  
{  
    printk( "call read -> buf : %08X, count : %08X \n", buf, count );  
    return 0x33;  
}  
  
ssize_t call_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)  
{  
    printk( "call write -> buf : %08X, count : %08X \n", buf, count );  
    return 0x43;  
}  
  
int call_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)  
{  
  
    printk( "call ioctl -> cmd : %08X, arg : %08X \n", cmd, arg );  
    return 0x53;  
}  
  
int call_release (struct inode *inode, struct file *filp)  
{  
    MOD_DEC_USE_COUNT;  
    printk( "call release \n" );  
    return 0;  
}  
  
struct file_operations call_fops =  
{  
    llseek   : call_llseek,     
    read     : call_read,       
    write    : call_write,      
    ioctl    : call_ioctl,      
    open     : call_open,       
    release  : call_release,    
};  
  
int init_module(void)  
{  
    int result;  
  
    printk( "call init_module\n" );      
  
    result = register_chrdev( CALL_DEV_MAJOR, CALL_DEV_NAME, &call_fops);  
    if (result < 0) return result;  
  
    return 0;  
}  
  
void cleanup_module(void)  
{  
    printk( "call cleanup_module\n" );      
    unregister_chrdev( CALL_DEV_MAJOR, CALL_DEV_NAME );  
}  
  
