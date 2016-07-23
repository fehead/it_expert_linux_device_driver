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

#define   INT_DEV_NAME            "intdev"
#define   INT_DEV_MAJOR            240

#define   INT_WRITE_ADDR        0x0378
#define   INT_READ_ADDR         0x0379
#define   INT_CTRL_ADDR         0x037A

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

#define   INT_BUFF_MAX              64

typedef struct   {
	unsigned long time;
} __attribute__ ((packed)) R_INT_INFO;

R_INT_INFO intbuffer[INT_BUFF_MAX];
int        intcount = 0;

void int_clear( void )
{
	int lp;

	for( lp = 0; lp < INT_BUFF_MAX; lp++ )
		intbuffer[lp].time  = 0;

	intcount = 0;
}

irqreturn_t int_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	if( intcount < INT_BUFF_MAX ) {
		intbuffer[intcount].time  = get_jiffies_64();
		intcount++;
	}
	return IRQ_HANDLED;
}

int int_open (struct inode *inode, struct file *filp)
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


	if( !request_irq( gpio_to_irq(27), int_interrupt, IRQF_DISABLED | IRQF_TRIGGER_FALLING, INT_DEV_NAME, NULL) ) {
		// 인터럽트 셋팅 27pin
		gpio_reg = readl( __io_address( GPFEN0_ADDR ));                        
		gpio_reg |= (1 << 27);
		writel( gpio_reg, __io_address( GPFEN0_ADDR ));                        
	}

	int_clear();

	return 0;
}

ssize_t int_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	int   readcount;
	char *ptrdata;
	int   loop;

	readcount = count / sizeof( R_INT_INFO );
	if( readcount > intcount ) readcount = intcount;

	ptrdata = (char * ) &intbuffer[0];

	for( loop = 0; loop < readcount * sizeof(R_INT_INFO); loop++ )
		put_user( ptrdata[loop], (char *) &buf[loop] );

	return readcount * sizeof( R_INT_INFO );
}

ssize_t int_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char status;
	int           loop;

	int_clear();

	unsigned char status;    
	get_user( status, (char *) buf ); 
	if(status == 0)
		writel( (1 << 17), __io_address( GPCLR0_ADDR ));
	else
		writel( (1 << 17), __io_address( GPSET0_ADDR ));

	return 1;
}

int int_release (struct inode *inode, struct file *filp)
{
	// 인터럽트 unset 27pin
	gpio_reg = readl( __io_address( GPFEN0_ADDR ));                        
	gpio_reg &= ~(1 << 27);
	writel( gpio_reg, __io_address( GPFEN0_ADDR ));                        

	free_irq( gpio_to_irq(27), NULL );
	return 0;
}

struct file_operations int_fops =
{
	.owner    = THIS_MODULE,
	.read     = int_read,
	.write    = int_write,
	.open     = int_open,
	.release  = int_release,
};

int int_init(void)
{
	int result;
	result = register_chrdev( INT_DEV_MAJOR, INT_DEV_NAME, &int_fops);
	if (result < 0) return result;

	return 0;
}

void int_exit(void)
{
	unregister_chrdev( INT_DEV_MAJOR, INT_DEV_NAME );
}

module_init(int_init);
module_exit(int_exit);

MODULE_LICENSE("Dual BSD/GPL");
