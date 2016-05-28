#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/kernel.h> 
  
#include <linux/fs.h>            
#include <linux/errno.h>         
#include <linux/types.h>         
#include <linux/fcntl.h>         
  
#include <asm/uaccess.h>  
#include <asm/io.h>  
  
#include "ioctl_test.h"  
  
#define   IOCTLTEST_DEV_NAME            "ioctldev"  
#define   IOCTLTEST_DEV_MAJOR            240  
  
#define   IOCTLTEST_WRITE_ADDR           0x0378  
#define   IOCTLTEST_READ_ADDR            0x0379  
  
int ioctltest_open (struct inode *inode, struct file *filp)  
{  
    return 0;  
}  
  
int ioctltest_release (struct inode *inode, struct file *filp)  
{  
    return 0;  
}  
  
int ioctltest_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)  
{  
  
    ioctl_test_info   ctrl_info;  
    int               err, size;  
    int               loop;  
      
    if( _IOC_TYPE( cmd ) != IOCTLTEST_MAGIC ) return -EINVAL;  
    if( _IOC_NR( cmd )   >= IOCTLTEST_MAXNR ) return -EINVAL;  
      
    size = _IOC_SIZE( cmd );   
      
    if( size )  
    {  
        err = 0;  
        if( _IOC_DIR( cmd ) & _IOC_READ  ) err = verify_area( VERIFY_WRITE, (void *) arg, size );  
        else if( _IOC_DIR( cmd ) & _IOC_WRITE ) err = verify_area( VERIFY_READ , (void *) arg, size );  
              
        if( err ) return err;          
    }  
            
    switch( cmd )  
    {  
    case IOCTLTEST_LEDOFF     : outb( 0x00 , IOCTLTEST_WRITE_ADDR );     
                                break;   
                                  
    case IOCTLTEST_LEDON      : outb( 0xFF , IOCTLTEST_WRITE_ADDR );     
                                break;  
                                  
    case IOCTLTEST_GETSTATE   : return inb( IOCTLTEST_READ_ADDR );   
      
    case IOCTLTEST_READ       : ctrl_info.buff[0] =  inb( IOCTLTEST_READ_ADDR );  
                                ctrl_info.size = 1;  
                                copy_to_user ( (void *) arg, (const void *) &ctrl_info, (unsigned long ) size );    
	                        break;  
	                          
    case IOCTLTEST_WRITE      : copy_from_user ( (void *)&ctrl_info, (const void *) arg, size );   
                                for( loop = 0; loop < ctrl_info.size; loop++ )   
                                    outb( ctrl_info.buff[loop] , IOCTLTEST_WRITE_ADDR );     
	                        break;  
	                          
    case IOCTLTEST_WRITE_READ : copy_from_user ( (void *)&ctrl_info, (const void *) arg, size );   
                                for( loop = 0; loop < ctrl_info.size; loop++ )   
                                    outb( ctrl_info.buff[loop] , IOCTLTEST_WRITE_ADDR );  
                                      
                                ctrl_info.buff[0] =  inb( IOCTLTEST_READ_ADDR );  
                                ctrl_info.size = 1;  
                                copy_to_user ( (void *) arg, (const void *) &ctrl_info, (unsigned long ) size );    
                                break;         
      
    }  
  
    return 0;  
}  
  
struct file_operations ioctltest_fops =  
{  
    .owner    = THIS_MODULE,  
    .ioctl    = ioctltest_ioctl,  
    .open     = ioctltest_open,       
    .release  = ioctltest_release,    
};  
  
int ioctltest_init(void)  
{  
    int result;  
  
    result = register_chrdev( IOCTLTEST_DEV_MAJOR, IOCTLTEST_DEV_NAME, &ioctltest_fops);  
    if (result < 0) return result;  
  
    return 0;  
}  
  
void ioctltest_exit(void)  
{  
    unregister_chrdev( IOCTLTEST_DEV_MAJOR, IOCTLTEST_DEV_NAME );  
}  
  
module_init(ioctltest_init);  
module_exit(ioctltest_exit);  
  
MODULE_LICENSE("Dual BSD/GPL");  
  
