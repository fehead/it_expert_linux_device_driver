#define MODULE
#include <linux/module.h>
#include <linux/kernel.h>      

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       

#include <asm/uaccess.h>
#include <asm/io.h>

#define   RDWR_DEV_NAME            "rdwrdev"
#define   RDWR_DEV_MAJOR            240      

#define   RDWR_WRITE_ADDR        0x0378      
#define   RDWR_READ_ADDR         0x0379      

int rdwr_open (struct inode *inode, struct file *filp)
{
    MOD_INC_USE_COUNT;
    return 0;
}

ssize_t rdwr_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    unsigned char status;
    int           loop;
    
    for( loop = 0; loop < count; loop++ )
    {
        status = inb( RDWR_READ_ADDR );
        put_user( status, (char *) &buf[loop] ); 
    }    

    return count;
}

ssize_t rdwr_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    unsigned char status;    
    int           loop;

    for( loop = 0; loop < count; loop++ )
    {
        get_user( status, (char *) buf ); 
        outb( status , RDWR_WRITE_ADDR );   
    }    
    return count;
}

int rdwr_release (struct inode *inode, struct file *filp)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

struct file_operations rdwr_fops =
{
    read     : rdwr_read,     
    write    : rdwr_write,    
    open     : rdwr_open,     
    release  : rdwr_release,  
};

int init_module(void)
{
    int result;

    result = register_chrdev( RDWR_DEV_MAJOR, RDWR_DEV_NAME, &rdwr_fops);
    if (result < 0) return result;

    return 0;
}

void cleanup_module(void)
{
    unregister_chrdev( RDWR_DEV_MAJOR, RDWR_DEV_NAME );
}

