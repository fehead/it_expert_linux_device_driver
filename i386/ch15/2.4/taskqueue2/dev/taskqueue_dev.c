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


typedef struct 
{
    unsigned long start_tick;
} Tledmng;

Tledmng ledmng;

void call_taskqueuefunc( void *data );
 
struct tq_struct taskqueue;
DECLARE_WAIT_QUEUE_HEAD( waitqueue_read );

void call_taskqueuefunc( void *data )
{
    Tledmng *mng;
    
    mng = (Tledmng *) data;
    
    if( jiffies - mng->start_tick > 300 )
    {
        wake_up_interruptible( &waitqueue_read );    
    }
    else
    {
        if( ( ( jiffies - mng->start_tick ) % 100 >= 50 ) ) outb( 0xFF , PRN_ADDRESS );   
        else                                                outb( 0x00 , PRN_ADDRESS );    
        queue_task(&taskqueue, &tq_timer);
    }
}

ssize_t taskqueue_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    ledmng.start_tick = jiffies;   
    queue_task(&taskqueue, &tq_timer);
    interruptible_sleep_on( &waitqueue_read );
    return 1;
}

int taskqueue_open (struct inode *inode, struct file *filp)
{
    MOD_INC_USE_COUNT;
    return 0;
}

int taskqueue_release (struct inode *inode, struct file *filp)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

struct file_operations taskqueue_fops =
{
    read    : taskqueue_read,    
    open    : taskqueue_open,     
    release : taskqueue_release,  
};

int taskqueue_init(void)
{
    int result;
    
    result = register_chrdev( DEV_MAJOR, DEV_NAME, &taskqueue_fops);
    if (result < 0) return result;

    taskqueue.routine = call_taskqueuefunc;
    taskqueue.data    = &ledmng;

    return 0;
}

void taskqueue_exit(void)
{
    unregister_chrdev( DEV_MAJOR, DEV_NAME );
}

module_init(taskqueue_init);
module_exit(taskqueue_exit);

