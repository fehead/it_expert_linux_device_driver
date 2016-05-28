#define MODULE
#include <linux/module.h>
#include <linux/kernel.h>      

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       

#include <asm/uaccess.h>
#include <asm/io.h>

#define   MINOR_DEV_NAME        "minordev"
#define   MINOR_DEV_MAJOR            240
#define   MINOR_WRITE_ADDR        0x0378
#define   MINOR_READ_ADDR         0x0379

int minor0_open (struct inode *inode, struct file *filp)
{
    printk( "call minor0_open\n" );
    MOD_INC_USE_COUNT;
    return 0;
}

ssize_t minor0_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    unsigned char status;    
    int           loop;

    for( loop = 0; loop < count; loop++ )
    {
        get_user( status, (char *) buf ); 
        outb( status , MINOR_WRITE_ADDR );   
    }    
    return count;
}

int minor0_release (struct inode *inode, struct file *filp)
{
    printk( "call minor0_release\n" );    
    MOD_DEC_USE_COUNT;
    return 0;
}

int minor1_open (struct inode *inode, struct file *filp)
{
    printk( "call minor1_open\n" );
    MOD_INC_USE_COUNT;
    return 0;
}

ssize_t minor1_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    unsigned char status;
    int           loop;
    
    for( loop = 0; loop < count; loop++ )
    {
        status = inb( MINOR_READ_ADDR );
        put_user( status, (char *) &buf[loop] );
    }

    return count;
}

int minor1_release (struct inode *inode, struct file *filp)
{
    printk( "call minor1_release\n" );
    MOD_DEC_USE_COUNT;
    return 0;
}

struct file_operations minor0_fops =
{
    write    : minor0_write,
    open     : minor0_open,
    release  : minor0_release,
};

struct file_operations minor1_fops =
{
    read     : minor1_read,
    open     : minor1_open,
    release  : minor1_release,
};

int minor_open (struct inode *inode, struct file *filp)
{
    printk( "call minor_open\n" );
    switch (MINOR(inode->i_rdev)) 
    {
    case 1: filp->f_op = &minor0_fops; break;
    case 2: filp->f_op = &minor1_fops; break;
    default : return -ENXIO;
    }
    
    if (filp->f_op && filp->f_op->open)
        return filp->f_op->open(inode,filp);
        
    return 0;
}

struct file_operations minor_fops =
{
    open     : minor_open,     
};

int init_module(void)
{
    int result;

    result = register_chrdev( MINOR_DEV_MAJOR, MINOR_DEV_NAME, &minor_fops);
    if (result < 0) return result;

    return 0;
}

void cleanup_module(void)
{
    unregister_chrdev( MINOR_DEV_MAJOR, MINOR_DEV_NAME );
}
