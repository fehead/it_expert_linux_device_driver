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

#include <linux/poll.h>

#define   POLL_DEV_NAME            "polldev"
#define   POLL_DEV_MAJOR            240

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

#define   poll_BUFF_MAX              64

DECLARE_WAIT_QUEUE_HEAD( WaitQueue_Read );

#define   MAX_QUEUE_CNT             128
static unsigned char ReadQ[MAX_QUEUE_CNT];
static unsigned long ReadQCount = 0;
static unsigned long ReadQHead  = 0;
static unsigned long ReadQTail  = 0;

irqreturn_t poll_interrupt(int irq, void *dev_id)
{
	unsigned long flags;

	/*
	local_save_flags(flags);
	local_irq_disable();
	*/
	local_irq_save(flags);

	if( ReadQCount < MAX_QUEUE_CNT ) {
		ReadQ[ReadQHead] = (unsigned long)readl( __io_address( GPLEV0_ADDR ));         
		ReadQHead = ( ReadQHead + 1 ) % MAX_QUEUE_CNT;
		ReadQCount++;
	}
	local_irq_restore(flags);

	wake_up_interruptible( &WaitQueue_Read );

	return IRQ_HANDLED;
}

int poll_open (struct inode *inode, struct file *filp)
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


	if( !request_irq( gpio_to_irq(27), poll_interrupt, IRQF_TRIGGER_FALLING, POLL_DEV_NAME, NULL) ) {
		/* 인터럽트 셋팅 27pin
		 * 27번째 핀에서 5v --> 0v로 떨어 질때 인터럽트가 발생하게 한다.
		 * gpio_reg = readl( __io_address( GPFEN0_ADDR ));                        
		 * gpio_reg |= (1 << 27);
		 */
		gpio_reg = (1 << 27);
		writel( gpio_reg, __io_address( GPFEN0_ADDR ));                        
	}
	return 0;
}

ssize_t poll_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	unsigned long flags;
	int           realmax;
	int           loop;
	int           retstate;

	if( (!ReadQCount) && (filp->f_flags & O_NONBLOCK) )
		return -EAGAIN;

	retstate = wait_event_interruptible( WaitQueue_Read, ReadQCount );
	if( retstate )
		return retstate;

	/*
	 * local_save_flags(flags);
	 * local_irq_disable();
	 */
	local_irq_save(flags);

	realmax = 0;
	if( ReadQCount >  0 ) {
		/* realmax = min(ReadQCount, count); */
		if( ReadQCount <= count )
			realmax = ReadQCount;
		else
			realmax = count;

		for( loop = 0; loop < realmax; loop++ ) {
			put_user( ReadQ[ReadQTail], (char *) &buf[loop] );
			ReadQTail = ( ReadQTail + 1 ) % MAX_QUEUE_CNT;
			ReadQCount--;
		}
	}
	local_irq_restore(flags);

	return realmax;
}

ssize_t poll_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char status;
	int           loop;

	for( loop = 0; loop < count; loop++ ) {
		get_user( status, (char *) buf );
		if(status == 0)
			writel( (1 << 17), __io_address( GPCLR0_ADDR )); // LED OFF
		else
			writel( (1 << 17), __io_address( GPSET0_ADDR )); // LED ON
	}

	return count;
}

unsigned int poll_poll( struct file *filp, poll_table *wait )
{
	unsigned int mask = 0;

	printk(KERN_INFO "poll_wait before....\n");
	poll_wait( filp, &WaitQueue_Read, wait );
	printk(KERN_INFO "poll_wait after....\n");

	if( ReadQCount > 0 )
		mask |= POLLIN | POLLRDNORM;

	return mask;
}


int poll_release (struct inode *inode, struct file *filp)
{
	u32     gpio_reg;

	// 인터럽트 unset 27pin
	gpio_reg = readl( __io_address( GPFEN0_ADDR ));                        
	gpio_reg &= ~(1 << 27);
	writel( gpio_reg, __io_address( GPFEN0_ADDR ));                        

	free_irq( gpio_to_irq(27), NULL );
	return 0;
}

struct file_operations poll_fops =
{
	.owner    = THIS_MODULE,
	.read     = poll_read,
	.write    = poll_write,
	.poll     = poll_poll,
	.open     = poll_open,
	.release  = poll_release,
};

int poll_init(void)
{
	int result;

	result = register_chrdev( POLL_DEV_MAJOR, POLL_DEV_NAME, &poll_fops);
	if (result < 0)
		return result;

	return 0;
}

void poll_exit(void)
{
	unregister_chrdev( POLL_DEV_MAJOR, POLL_DEV_NAME );
}

module_init(poll_init);
module_exit(poll_exit);

MODULE_LICENSE("Dual BSD/GPL");

