#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>      

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>  
     
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/devfs_fs_kernel.h>

#define   DEVFS_DEV_NAME            "devfs_ex"
#define   DEVFS_DEV_MAJOR            240      

int devfs_open (struct inode *inode, struct file *filp)
{
    return 0;
}

int devfs_release (struct inode *inode, struct file *filp)
{
    return 0;
}

struct file_operations devfs_fops =
{
    .owner    = THIS_MODULE,
    .open     = devfs_open,
    .release  = devfs_release,  
};

int devfs_init(void)
{
    int result;
    int lp;

    result = register_chrdev( DEVFS_DEV_MAJOR, DEVFS_DEV_NAME, &devfs_fops);
    if (result < 0) return result;

    devfs_mk_dir( "devfs_test" );
    devfs_mk_dir( "devfs_test/subdir" );
    devfs_mk_cdev( MKDEV(DEVFS_DEV_MAJOR,0),  S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP,  "devfs_test/devfs_main" );
    devfs_mk_symlink( "devfs_test/devfs_link", "devfs_main" );
    
    for( lp = 1; lp < 10; lp++ )
    {
        devfs_mk_cdev( MKDEV(DEVFS_DEV_MAJOR,lp),  S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP,  "devfs_test/devfs_sub%02d", lp );
    }

    return 0;
}

void devfs_exit(void)
{
    int lp;

    for( lp = 1; lp < 10; lp++ )
    {
        devfs_remove( "devfs_test/devfs_sub%02d", lp );
    }
        
    devfs_remove( "devfs_test/devfs_link" );        
    devfs_remove( "devfs_test/devfs_main" );    
    devfs_remove( "devfs_test/subdir" );
    devfs_remove( "devfs_test" );
        
    unregister_chrdev( DEVFS_DEV_MAJOR, DEVFS_DEV_NAME );
}

module_init(devfs_init);
module_exit(devfs_exit);

MODULE_LICENSE("Dual BSD/GPL");
