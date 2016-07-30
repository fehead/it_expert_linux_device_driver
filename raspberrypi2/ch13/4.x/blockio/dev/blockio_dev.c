#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/platform.h>
#include <asm/gpio.h>

#include <linux/time.h>
#include <linux/timer.h>

#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define   BLOCKIO_DEV_NAME            "blockiodev"
#define   BLOCKIO_DEV_MAJOR            240

#define   BLOCKIO_WRITE_ADDR        0x0378
#define   BLOCKIO_READ_ADDR         0x0379
#define   BLOCKIO_CTRL_ADDR         0x037A

#define GPFSEL1_ADDR    (GPIO_BASE + 0x04)
#define GPFSEL2_ADDR    (GPIO_BASE + 0x08)

/* IAMROOT-12CD (2016-07-02):
 * --------------------------
 * GPIO_BASE = BCM2708_PERI_BASE + GPIO_BASE
 *	= 0x3F000000 + 0x200000
 *	= 0x3F200000
 * GPFSEL1_ADDR = 0x3F200004
 * GPFSEL2_ADDR = 0x3F200008
 */
#define GPSET0_ADDR     (GPIO_BASE + 0x1c)
#define GPCLR0_ADDR     (GPIO_BASE + 0x28)
#define GPLEV0_ADDR     (GPIO_BASE + 0x34)
#define GPFEN0_ADDR     (GPIO_BASE + 0x58)

#define   BLOCKIO_BUFF_MAX              64

typedef struct   {
	unsigned long time;
} __attribute__ ((packed)) R_BLOCKIO_INFO;

R_BLOCKIO_INFO intbuffer[BLOCKIO_BUFF_MAX];
int        intcount = 0;

DECLARE_WAIT_QUEUE_HEAD( WaitQueue_Read );
int cond;

void blockio_clear( void )
{
	int lp;

	for( lp = 0; lp < BLOCKIO_BUFF_MAX; lp++ )
		intbuffer[lp].time  = 0;

	intcount = 0;
}

irqreturn_t blockio_interrupt(int irq, void *dev_id)
{
	if( intcount < BLOCKIO_BUFF_MAX ) {
		intbuffer[intcount].time  = get_jiffies_64();
		intcount++;
	}
	cond = 1;
	wake_up_interruptible( &WaitQueue_Read );
	return IRQ_HANDLED;
}

int blockio_open (struct inode *inode, struct file *filp)
{
	u32     gpio_reg;

	// Output(GPIO 17) --> SEL1, 21~23 bit
	gpio_reg = readl( __io_address( GPFSEL1_ADDR ));
	gpio_reg &= ~(0x07 << (3*7));	// 21~23 bit
	gpio_reg |= (0x01 << (3*7));	// output set 001
	/* IAMROOT-12CD (2016-07-02):
	 * --------------------------
	 * GPFSEL1_ADDR = 0x3F200004
	 * (*0x3F200004) = gpio_reg;
	 */
	writel( gpio_reg, __io_address( GPFSEL1_ADDR ));

	// Input(GPIO 27) --> SEL2, 21~23 bit                                   
	gpio_reg = readl( __io_address( GPFSEL2_ADDR ));                        
	gpio_reg &= ~(0x07 << (3*7));                 
	gpio_reg |= (0x00 << (3*7));	// input set 000
	/* IAMROOT-12CD (2016-07-02):
	 * --------------------------
	 * GPFSEL2_ADDR = 0x3F200008
	 * (*0x3F200008) = gpio_reg;
	 */
	writel( gpio_reg, __io_address( GPFSEL2_ADDR ));                        


	if( !request_irq( gpio_to_irq(27), blockio_interrupt, IRQF_TRIGGER_FALLING, BLOCKIO_DEV_NAME, NULL) ) {
		/* 인터럽트 셋팅 27pin
		 * 27번째 핀에서 5v --> 0v로 떨어 질때 인터럽트가 발생하게 한다.
		 * gpio_reg = readl( __io_address( GPFEN0_ADDR ));                        
		 * gpio_reg |= (1 << 27);
		 */
		gpio_reg = (1 << 27);
		writel( gpio_reg, __io_address( GPFEN0_ADDR ));                        
	}

	blockio_clear();

	return 0;
}

ssize_t blockio_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	int   readcount;
	char *ptrdata;
	int   loop;

	if( !intcount ) {
		if( !(filp->f_flags & O_NONBLOCK) ) {
			cond = 0;
			wait_event_interruptible( WaitQueue_Read, cond );
		}
		else
			return -EAGAIN;
	}

	readcount = count / sizeof( R_BLOCKIO_INFO );
	if( readcount > intcount ) readcount = intcount;

	ptrdata = (char * ) &intbuffer[0];

	for( loop = 0; loop < readcount * sizeof(R_BLOCKIO_INFO); loop++ )
		put_user( ptrdata[loop], (char *) &buf[loop] );

	return readcount * sizeof( R_BLOCKIO_INFO );
}

ssize_t blockio_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char status;

	blockio_clear();

	get_user( status, (char *) buf ); 
	if(status == 0)
		writel( (1 << 17), __io_address( GPCLR0_ADDR ));
	else
		writel( (1 << 17), __io_address( GPSET0_ADDR ));

	return 1;
}

int blockio_release (struct inode *inode, struct file *filp)
{
	u32     gpio_reg;

	// 인터럽트 unset 27pin
	gpio_reg = readl( __io_address( GPFEN0_ADDR ));                        
	gpio_reg &= ~(1 << 27);
	writel( gpio_reg, __io_address( GPFEN0_ADDR ));                        

	free_irq( gpio_to_irq(27), NULL );
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
