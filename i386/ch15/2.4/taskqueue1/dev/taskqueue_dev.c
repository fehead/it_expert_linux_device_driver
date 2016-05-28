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
#include <linux/sysctl.h>

#include <linux/tqueue.h>


#define   DEV_NAME                  "taskqueue"   
#define   DEV_MAJOR                        240    

#define   PRN_ADDRESS_RANGE                  3      
#define   PRN_ADDRESS                   0x0378      
#define   PRN_ADDRESS_STATE             0x0379      
#define   PRN_ADDRESS_CTRL              0x037A      

#define   PRN_IRQ_ENABLE_MASK             0x10      
#define   PRN_IRQ                            7      


unsigned long led_flash_count = 0;

void call_taskqueuefunc( void *data );
 
struct tq_struct taskqueue;
DECLARE_WAIT_QUEUE_HEAD( waitqueue_read );

void call_taskqueuefunc( void *data )
{
    printk( "[TASK QUEUE] Enter taskqueue function\n" );
}

void taskqueue_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    printk( "[TASK QUEUE] Enter taskqueue_interrupt in interrupt\n" );
    queue_task(&taskqueue, &tq_timer);
    wake_up_interruptible( &waitqueue_read );
    printk( "[TASK QUEUE] After queue_task in interrupt\n" );
}

ssize_t taskqueue_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    printk( "[TASK QUEUE] Enter taskqueue_read in read\n" );    
    queue_task(&taskqueue, &tq_timer);
    interruptible_sleep_on( &waitqueue_read );
    printk( "[TASK QUEUE] After queue_task in read\n" );    
    return 1;
}

ssize_t taskqueue_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    printk( "[TASK QUEUE] Enter taskqueue_write in write\n" );        
    queue_task(&taskqueue, &tq_timer);
    printk( "[TASK QUEUE] After queue_task in write\n" );    
    return 1;
}

int taskqueue_open (struct inode *inode, struct file *filp)
{
    if( !request_irq( PRN_IRQ , taskqueue_interrupt, SA_INTERRUPT, DEV_NAME, NULL) )
    {
	outb( PRN_IRQ_ENABLE_MASK, PRN_ADDRESS_CTRL );
    }

    MOD_INC_USE_COUNT;
    return 0;
}

int taskqueue_release (struct inode *inode, struct file *filp)
{
    outb( 0x00, PRN_ADDRESS_CTRL );
    free_irq( PRN_IRQ , NULL );

    MOD_DEC_USE_COUNT;
    return 0;
}

struct file_operations taskqueue_fops =
{
    read    : taskqueue_read,    
    write   : taskqueue_write,
    open    : taskqueue_open,     
    release : taskqueue_release,  
};

int taskqueue_init(void)
{
    int result;
    
    result = register_chrdev( DEV_MAJOR, DEV_NAME, &taskqueue_fops);
    if (result < 0) return result;

    taskqueue.routine = call_taskqueuefunc;
    taskqueue.data    = NULL;          

    return 0;
}

void taskqueue_exit(void)
{
    unregister_chrdev( DEV_MAJOR, DEV_NAME );
}

module_init(taskqueue_init);
module_exit(taskqueue_exit);

