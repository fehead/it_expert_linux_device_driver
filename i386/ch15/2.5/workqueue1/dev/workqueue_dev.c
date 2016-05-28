#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/pnp.h>
#include <linux/sysctl.h>

#include <linux/timer.h>
#include <linux/workqueue.h>

#include <asm/io.h>
#include <asm/dma.h>
#include <asm/uaccess.h>


#define   DEV_NAME                  "workqueue"   
#define   DEV_MAJOR                        240    

#define   PRN_ADDRESS_RANGE                  3      
#define   PRN_ADDRESS                   0x0378      
#define   PRN_ADDRESS_STATE             0x0379      
#define   PRN_ADDRESS_CTRL              0x037A      

#define   PRN_IRQ_ENABLE_MASK             0x10      
#define   PRN_IRQ                            7      


void call_workqueuefunc( void *data );
 
DECLARE_WAIT_QUEUE_HEAD( waitqueue_read );
DECLARE_WORK(work_queue, call_workqueuefunc, NULL );

void call_workqueuefunc( void *data )
{
    printk( "[WORK QUEUE] Enter workqueue function\n" );
}

irqreturn_t workqueue_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    printk( "[WORK QUEUE] Enter workqueue_interrupt in interrupt\n" );
    schedule_work( &work_queue );
    wake_up_interruptible( &waitqueue_read );
    printk( "[WORK QUEUE] After schedule_work in interrupt\n" );
    return IRQ_HANDLED;
}

ssize_t workqueue_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    printk( "[WORK QUEUE] Enter workqueue_read in read\n" );    
    schedule_work( &work_queue );    
    interruptible_sleep_on( &waitqueue_read );
    printk( "[WORK QUEUE] After schedule_work in read\n" );    
    return 1;
}

ssize_t workqueue_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    printk( "[WORK QUEUE] Enter workqueue_write in write\n" );        
    schedule_work( &work_queue );
    printk( "[WORK QUEUE] After schedule_work in write\n" );    
    return 1;
}

int workqueue_open (struct inode *inode, struct file *filp)
{
    if( !request_irq( PRN_IRQ , workqueue_interrupt, SA_INTERRUPT, DEV_NAME, NULL) )
    {
	outb( PRN_IRQ_ENABLE_MASK, PRN_ADDRESS_CTRL );
    }

    try_module_get (THIS_MODULE);
    return 0;
}

int workqueue_release (struct inode *inode, struct file *filp)
{
    outb( 0x00, PRN_ADDRESS_CTRL );
    free_irq( PRN_IRQ , NULL );

    module_put (THIS_MODULE);
    return 0;
}

struct file_operations workqueue_fops =
{
    .owner   = THIS_MODULE,        
    .read    = workqueue_read,    
    .write   = workqueue_write,
    .open    = workqueue_open,     
    .release = workqueue_release,  
};


int workqueue_init(void)
{
    int result;
    
    result = register_chrdev( DEV_MAJOR, DEV_NAME, &workqueue_fops);
    if (result < 0) return result;

    return 0;
}

void workqueue_exit(void)
{
    unregister_chrdev( DEV_MAJOR, DEV_NAME );
}

module_init(workqueue_init);
module_exit(workqueue_exit);

MODULE_LICENSE("Dual BSD/GPL");
