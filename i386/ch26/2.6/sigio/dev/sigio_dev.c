#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       

#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/time.h>
#include <linux/timer.h>

#include <linux/interrupt.h>
#include <linux/wait.h>

#define   SIGIO_DEV_NAME            "sigiodev"
#define   SIGIO_DEV_MAJOR            240      

#define   SIGIO_CTRL_ADDR         0x037A   

#define   SIGIO_IRQ                    7
#define   SIGIO_IRQ_ENABLE_MASK     0x10   

int                   intcount = 0;
struct fasync_struct *sigio_async_queue;

irqreturn_t sigio_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    intcount++;    
    kill_fasync (&sigio_async_queue, SIGIO, POLL_IN);
    return IRQ_HANDLED; 
}

int sigio_fasync (int fd, struct file *filp, int mode )
{
    return fasync_helper (fd, filp, mode, &sigio_async_queue);
}

int sigio_open (struct inode *inode, struct file *filp)
{
    if( !request_irq( SIGIO_IRQ , sigio_interrupt, SA_INTERRUPT, SIGIO_DEV_NAME, NULL) )
    {
        outb( SIGIO_IRQ_ENABLE_MASK, SIGIO_CTRL_ADDR );
    }
    return 0;
}

ssize_t sigio_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    char one;
    
    one = (intcount&0xF) + 'A';
    put_user( one, (char *) &buf[0] ); 
    return 1;
}

int sigio_release (struct inode *inode, struct file *filp)
{
    sigio_fasync(-1, filp, 0);
        
    outb( 0x00, SIGIO_CTRL_ADDR ); 
    free_irq( SIGIO_IRQ , NULL );
    return 0;
}

struct file_operations sigio_fops =
{
    .owner    = THIS_MODULE,
    .read     = sigio_read,
    .open     = sigio_open,
    .release  = sigio_release,  
    .fasync   = sigio_fasync, 
};

int sigio_init(void)
{
    int result;

    result = register_chrdev( SIGIO_DEV_MAJOR, SIGIO_DEV_NAME, &sigio_fops);
    if (result < 0) return result;
    
    return 0;
}

void sigio_exit(void)
{
    unregister_chrdev( SIGIO_DEV_MAJOR, SIGIO_DEV_NAME );
}

module_init(sigio_init);
module_exit(sigio_exit);

MODULE_LICENSE("Dual BSD/GPL");

