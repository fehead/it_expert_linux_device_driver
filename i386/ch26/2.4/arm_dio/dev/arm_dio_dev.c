#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>      
#include <linux/version.h>
#include <linux/init.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/ioport.h>

#include <linux/poll.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware.h>

#define   ARMDIO_DEV_MAJOR     240 
#define   ARMDIO_DEV_NAME      "armdio_dev"

#define   DIO_BASE_ADDR          0xF4000000                       // nCS4
#define   DIO_WRITE_ADDR        (DIO_BASE_ADDR      )             // 출력 포트 
#define   DIO_READ_OFFSET       (DIO_BASE_ADDR+0x100)             // 입력 포트
#define   DIO_REGION             0x200                            // I/O 접근 범위

#define   TIMER_TICK_100uSEC     10                               // 10 마이크로 단위의 틱 시간 간격 

#define   TEN_USEC_CLK           (3686400/100000)
#define   SATIMER_ON             OIER |=  OIER_E1;                
#define   SATIMER_OFF            OIER &= ~OIER_E1; OSSR = OSSR_M1 

#define   DI_SCAN_TIME           200  // 20 msec
#define   DO_DEFAULT_TIME        500  // 20 msec
#define   MAX_READ_QUEUE_CNT     128
#define   MAX_WRITE_CNT          256

#define   ARMDIO_IOCTL_MAGIC    'a'
#define   ARMDIO_IOCTL_SET_MODE _IOW( ARMDIO_IOCTL_MAGIC, 0 , unsigned long )
#define   ARMDIO_IOCTL_SET_TIME _IOW( ARMDIO_IOCTL_MAGIC, 1 , unsigned long )
#define   ARMDIO_IOCTL_MAXNR     2

typedef struct
{
    wait_queue_head_t waitqueue_read;
    unsigned long     read_cnt;
    unsigned short    di_last;
    unsigned char     read_q[MAX_READ_QUEUE_CNT]; 
    unsigned long     read_q_cnt;
    unsigned long     read_q_head;
    unsigned long     read_q_tail;

    wait_queue_head_t waitqueue_write;
    unsigned long     write_int_flag;
    unsigned long     write_cnt;
    unsigned long     do_mode;
    unsigned long     do_time;
        
    unsigned long     do_buff[MAX_WRITE_CNT];
    unsigned long     do_cnt;
    unsigned long     do_index;
    
} __attribute__ ((packed)) armdio_mng_t;

static int armdio_usage =  0;

static void armdio_time_reload( unsigned long  tick )
{
    if( 0 == tick ) 
    {
        SATIMER_OFF;    
    }
    else
    {
        SATIMER_ON;
        OSMR1 = OSCR + TEN_USEC_CLK*tick; // reload
    }
}

void armdio_int_read_handler(armdio_mng_t *data)
{
    unsigned long  flags;    
    unsigned short state;
    unsigned char  key;
    int            lp;
    
    state = readw( DIO_READ_OFFSET ) & 0xFF;
    if( data->di_last == state ) return;
    
    for( lp = 0; lp < 8; lp++ )
    {
        if( (data->di_last&(1<<lp)) != (state&(1<<lp)) )
        {
             key = '0' + lp;   
             if( (state&(1<<lp)) ) key |= 0x80;
             save_flags_cli(flags);  
             if( data->read_q_cnt < MAX_READ_QUEUE_CNT )
             {
                 data->read_q[data->read_q_head] = key;
                 data->read_q_head = ( data->read_q_head + 1 ) % MAX_READ_QUEUE_CNT;
                 data->read_q_cnt++;
             }
             restore_flags(flags);
         }           
    }        
    data->di_last = state;
    
    wake_up_interruptible( &(data->waitqueue_read) );
}

void armdio_int_write_handler(armdio_mng_t *data)
{
    unsigned long  flags;    
    unsigned short state;
    int            wake_flag;
    
    data->write_cnt++;
    if( data->write_cnt < data->do_time ) return;
    data->write_cnt = 0;
    if( !data->do_cnt ) return;

    save_flags_cli(flags);  
    writew( data->do_buff[data->do_index], DIO_WRITE_ADDR );
    data->do_index++;
    if( data->do_index >= data->do_cnt )
    {
        data->write_int_flag = 0;
        wake_up_interruptible( &(data->waitqueue_write) );
    }
    restore_flags(flags);
    
}

void armdio_int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    armdio_mng_t   *data;    
    unsigned long  flags;
    unsigned short state;

    data = (armdio_mng_t *) dev_id;
    
    SATIMER_OFF;
    armdio_time_reload( TIMER_TICK_100uSEC );
    
    if( data->write_int_flag ) armdio_int_write_handler(data);
    
    data->read_cnt++;
    if( data->read_cnt >= DI_SCAN_TIME )
    {
        data->read_cnt = 0;
        armdio_int_read_handler(data);
    }
}

static __init int armdio_probe(void)
{
    return 1;
}

void armdio_hw_init( void )
{
    writew( 0x0000, DIO_WRITE_ADDR );
}

int armdio_open (struct inode *inode, struct file *filp)
{
    int           ret;
    armdio_mng_t *data;
    
    ret = 0;
    
    if( armdio_usage != 0 ){ ret = -EBUSY; goto err1; }
    armdio_usage++;
    
    if( check_region( DIO_BASE_ADDR, DIO_REGION ) ) { ret = -ENODEV; goto err2; }
    request_region( DIO_BASE_ADDR, DIO_REGION, ARMDIO_DEV_NAME );

    data = (armdio_mng_t *) kmalloc( sizeof( armdio_mng_t ), GFP_KERNEL ); 
    
    if( data == NULL ) { ret = -ENOMEM; goto err3; }
    memset( data, 0, sizeof( armdio_mng_t ) );
    
    init_waitqueue_head( &(data->waitqueue_read) );
    init_waitqueue_head( &(data->waitqueue_write) );
    
    data->di_last = readw( DIO_READ_OFFSET );
    data->do_time = DO_DEFAULT_TIME;
    
    filp->private_data = (void *) data;
    
    ret = request_irq( IRQ_OST1, armdio_int_handler, SA_INTERRUPT, ARMDIO_DEV_NAME, filp->private_data );
    if( ret ) goto err4;
    armdio_time_reload( TIMER_TICK_100uSEC );
    
    MOD_INC_USE_COUNT;
    return 0;

err4: kfree( data );    
err3: release_region( DIO_BASE_ADDR, DIO_REGION );
err2: armdio_usage--;
err1:     
      return ret;
}

int armdio_release (struct inode *inode, struct file *filp)
{
    armdio_mng_t *data;
    
    data = (armdio_mng_t *) filp->private_data;
        
    kfree( data );

    SATIMER_OFF;
    free_irq( IRQ_OST1, data );

    release_region( DIO_BASE_ADDR, DIO_REGION );

    armdio_usage--;     
    MOD_DEC_USE_COUNT;  

    return 0;
}

ssize_t armdio_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    int           ret; 
    unsigned long flags;
    armdio_mng_t *data;
    int           realmax;
    int           lp;
 
    data = (armdio_mng_t *) filp->private_data;

    if( (!data->read_q_cnt) && (filp->f_flags & O_NONBLOCK) ) return -EAGAIN;
    ret = wait_event_interruptible( data->waitqueue_read, data->read_q_cnt ); 
    if( ret ) return ret;
    
    save_flags_cli(flags);
    realmax = 0;
    if( data->read_q_cnt >  0 )
    {
        if( data->read_q_cnt <= count ) realmax = data->read_q_cnt;
        else                            realmax = count;
        
        for( lp = 0; lp < realmax; lp++ )
        {
            put_user( data->read_q[data->read_q_tail], (char *) &buf[lp] );
            data->read_q_tail = ( data->read_q_tail + 1 ) % MAX_READ_QUEUE_CNT;
            data->read_q_cnt--;
        }
    }
    restore_flags(flags);
    
    return realmax;
}

ssize_t armdio_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    armdio_mng_t *data;
    unsigned long flags;
    unsigned char state;
    int           realmax;
    int           lp;
    int           ret;
    
    data = (armdio_mng_t *) filp->private_data;
    
    if( data->do_mode == 0 )
    {
        get_user( state, (char *) buf ); 
        writew( state, DIO_WRITE_ADDR );
        return count;
    }
    else
    {
        if( !count )
        {
          data->do_mode         = 0;
          data->write_int_flag  = 0;
          return 0;
        }

        ret = wait_event_interruptible( data->waitqueue_write, !data->write_int_flag ); 
        if( ret ) return ret;

        realmax = count;
        if( realmax >= MAX_WRITE_CNT ) realmax = MAX_WRITE_CNT;
        for( lp = 0; lp < realmax; lp++ )
        {
            get_user( data->do_buff[lp], (char *) &buf[lp] );   
        }
        save_flags_cli(flags);
        data->do_index = 0;   
        data->do_cnt   = realmax;
        data->write_int_flag = 1;
        restore_flags(flags);
    }
    
    return count;
}

int armdio_ioctl (struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)  
{  
    armdio_mng_t *data;
    
    data = (armdio_mng_t *) filp->private_data;
      
    if( _IOC_TYPE( cmd ) != ARMDIO_IOCTL_MAGIC ) return -EINVAL;  
    if( _IOC_NR( cmd )   >= ARMDIO_IOCTL_MAXNR ) return -EINVAL;  

    switch( cmd )  
    {  
    case ARMDIO_IOCTL_SET_MODE : data->do_mode = arg; 
                                 if( data->do_mode ) 
                                 { 
                                     data->do_index = 0;   
                                     data->do_cnt   = 0;
                                 }
                                 data->write_int_flag  = 0;   
                                 break;      
    case ARMDIO_IOCTL_SET_TIME : data->do_time = arg; 
                                 break;  
    }  
  
    return 0;  
}  

unsigned int armdio_poll( struct file *filp, poll_table *wait )
{
    armdio_mng_t *data;
    unsigned int mask;

    data = (armdio_mng_t *) filp->private_data;
    
    poll_wait( filp, &(data->waitqueue_write), wait );
    poll_wait( filp, &(data->waitqueue_read), wait );
        
    mask = 0;    
    if(data->read_q_cnt) mask |= (POLLIN | POLLRDNORM); 
    if(data->do_mode && data->do_cnt)    
    {
       if( !data->write_int_flag ) mask |= (POLLOUT| POLLWRNORM);
    }    

    return mask;
}

struct file_operations armdio_fops =
{
    open     : armdio_open,
    release  : armdio_release,
    read     : armdio_read,
    write    : armdio_write,
    ioctl    : armdio_ioctl,
    poll     : armdio_poll,
};

static __init int armdio_init(void)
{
    int result;

    if( !armdio_probe() ) return -ENODEV;

    result = register_chrdev( ARMDIO_DEV_MAJOR, ARMDIO_DEV_NAME, &armdio_fops);
    if (result < 0) return result;
    
    armdio_hw_init();
 
    return 0;
}

static __exit void armdio_exit(void)
{
    unregister_chrdev( ARMDIO_DEV_MAJOR, ARMDIO_DEV_NAME );
}

module_init(armdio_init);
module_exit(armdio_exit);

MODULE_LICENSE("Dual BSD/GPL");
