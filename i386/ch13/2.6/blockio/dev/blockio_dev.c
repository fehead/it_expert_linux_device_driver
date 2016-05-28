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

#define   BLOCKIO_DEV_NAME            "blockiodev"
#define   BLOCKIO_DEV_MAJOR            240      

#define   BLOCKIO_WRITE_ADDR        0x0378      
#define   BLOCKIO_READ_ADDR         0x0379      
#define   BLOCKIO_CTRL_ADDR         0x037A   

#define   BLOCKIO_IRQ                    7
#define   BLOCKIO_IRQ_ENABLE_MASK     0x10   

#define   BLOCKIO_BUFF_MAX              64

typedef struct 
{
    unsigned long time;
} __attribute__ ((packed)) R_BLOCKIO_INFO;

R_BLOCKIO_INFO intbuffer[BLOCKIO_BUFF_MAX];
int        intcount = 0;

DECLARE_WAIT_QUEUE_HEAD( WaitQueue_Read );          // 읽기에 대한 블럭 모드 구현을 위한 대기 큐 변수  

void blockio_clear( void )
{
    int lp;

    for( lp = 0; lp < BLOCKIO_BUFF_MAX; lp++ )
    {
        intbuffer[lp].time  = 0;
    }
    
    intcount = 0;
    
}

irqreturn_t blockio_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    if( intcount < BLOCKIO_BUFF_MAX )
    {
        intbuffer[intcount].time  = get_jiffies_64();
        intcount++;
    }
    
    wake_up_interruptible( &WaitQueue_Read );
    
    return IRQ_HANDLED; 
}

int blockio_open (struct inode *inode, struct file *filp)
{
    if( !request_irq( BLOCKIO_IRQ , blockio_interrupt, SA_INTERRUPT, BLOCKIO_DEV_NAME, NULL) )
    {
        outb( BLOCKIO_IRQ_ENABLE_MASK, BLOCKIO_CTRL_ADDR );
    }
    
    blockio_clear();

    return 0;
}

ssize_t blockio_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    int   readcount;
    char *ptrdata;
    int   loop;

    if( !intcount )
    {
        if( !(filp->f_flags & O_NONBLOCK) )
        {
           interruptible_sleep_on( &WaitQueue_Read ); 
        }
        else
        {
           return -EAGAIN;
        }   
    }
    
    readcount = count / sizeof( R_BLOCKIO_INFO );
    if( readcount > intcount ) readcount = intcount;
    
    ptrdata = (char * ) &intbuffer[0];
    
    for( loop = 0; loop < readcount * sizeof(R_BLOCKIO_INFO); loop++ )
    {
        put_user( ptrdata[loop], (char *) &buf[loop] ); 
    }
    
    return readcount * sizeof( R_BLOCKIO_INFO );
}

ssize_t blockio_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    unsigned char status;
    int           loop;

    blockio_clear();
    
    for( loop = 0; loop < count; loop++ )
    {
        get_user( status, (char *) buf ); 
        outb( status , BLOCKIO_WRITE_ADDR );   
    }    
    
    return count;
}

int blockio_release (struct inode *inode, struct file *filp)
{
    outb( 0x00, BLOCKIO_CTRL_ADDR ); 
    free_irq( BLOCKIO_IRQ , NULL );
    return 0;
}

struct file_operations blockio_fops =
{
    .owner    = THIS_MODULE,
    .read     = blockio_read,
    .write    = blockio_write,
    .open     = blockio_open,
    .release  = blockio_release,  
};

int blockio_init(void)
{
    int result;

    result = register_chrdev( BLOCKIO_DEV_MAJOR, BLOCKIO_DEV_NAME, &blockio_fops);
    if (result < 0) return result;
    
    return 0;
}

void blockio_exit(void)
{
    unregister_chrdev( BLOCKIO_DEV_MAJOR, BLOCKIO_DEV_NAME );
}

module_init(blockio_init);
module_exit(blockio_exit);

MODULE_LICENSE("Dual BSD/GPL");

