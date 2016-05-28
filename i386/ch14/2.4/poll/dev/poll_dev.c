#define MODULE
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

#include <linux/poll.h>

#define   POLL_DEV_NAME            "polldev"
#define   POLL_DEV_MAJOR            240

#define   POLL_WRITE_ADDR        0x0378
#define   POLL_READ_ADDR         0x0379
#define   POLL_CTRL_ADDR         0x037A

#define   POLL_IRQ                    7
#define   POLL_IRQ_ENABLE_MASK     0x10

#define   poll_BUFF_MAX              64

DECLARE_WAIT_QUEUE_HEAD( WaitQueue_Read );

#define   MAX_QUEUE_CNT             128
static unsigned char ReadQ[MAX_QUEUE_CNT];          
static unsigned long ReadQCount = 0;                
static unsigned long ReadQHead  = 0;                
static unsigned long ReadQTail  = 0;                

void poll_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    unsigned long flags;

    save_flags(flags);
    cli();

    if( ReadQCount < MAX_QUEUE_CNT )
    {
        ReadQ[ReadQHead] = (unsigned long) inb( POLL_READ_ADDR );
        ReadQHead = ( ReadQHead + 1 ) % MAX_QUEUE_CNT;
        ReadQCount++;
    }
    restore_flags(flags);

    wake_up_interruptible( &WaitQueue_Read );
}

int poll_open (struct inode *inode, struct file *filp)
{
    if( !request_irq( POLL_IRQ , poll_interrupt, SA_INTERRUPT, POLL_DEV_NAME, NULL) )
    {
        outb( POLL_IRQ_ENABLE_MASK, POLL_CTRL_ADDR );
    }
    return 0;
}

ssize_t poll_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    unsigned long flags;
    int           realmax;
    int           loop;
    int           retstate; 
    
    if( (!ReadQCount) && (filp->f_flags & O_NONBLOCK) ) return -EAGAIN;

    retstate = wait_event_interruptible( WaitQueue_Read, ReadQCount ); 
    if( retstate ) return retstate;
    
    save_flags(flags);
    cli();    
    
    realmax = 0;
    if( ReadQCount >  0 )
    {
        if( ReadQCount <= count ) realmax = ReadQCount;
        else                      realmax = count;
        
        for( loop = 0; loop < realmax; loop++ )
        {
            put_user( ReadQ[ReadQTail], (char *) &buf[loop] );
            ReadQTail = ( ReadQTail + 1 ) % MAX_QUEUE_CNT;
            ReadQCount--;
        }
    }
    restore_flags(flags);
        
    return realmax;
}

ssize_t poll_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    unsigned char status;
    int           loop;

    for( loop = 0; loop < count; loop++ )
    {
        get_user( status, (char *) buf ); 
        outb( status , POLL_WRITE_ADDR );   
    }    
    
    return count;
}

unsigned int poll_poll( struct file *filp, poll_table *wait )
{
	unsigned int mask = 0;

        poll_wait( filp, &WaitQueue_Read, wait );
        
        if( ReadQCount > 0 ) mask |= POLLIN | POLLRDNORM;
        
	return mask;
}	


int poll_release (struct inode *inode, struct file *filp)
{
    outb( 0x00, POLL_CTRL_ADDR ); 
    free_irq( POLL_IRQ , NULL );
    return 0;
}

struct file_operations poll_fops =
{
    read     : poll_read,
    write    : poll_write,
    poll     : poll_poll,
    open     : poll_open,
    release  : poll_release,  
};

int init_module(void)
{
    int result;

    result = register_chrdev( POLL_DEV_MAJOR, POLL_DEV_NAME, &poll_fops);
    if (result < 0) return result;
    
    return 0;
}

void cleanup_module(void)
{
    unregister_chrdev( POLL_DEV_MAJOR, POLL_DEV_NAME );
}

