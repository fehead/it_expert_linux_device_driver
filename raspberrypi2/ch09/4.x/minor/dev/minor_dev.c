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

#define   MINOR_DEV_NAME        "minordev"
#define   MINOR_DEV_MAJOR            240

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
#define GPIO_LED_NR     7               /* GPIO 17, 27 pin */                   
#define GPIO_LED_GR     0               /* GPIO Base Pin */                     
                                                                 
int minor0_open (struct inode *inode, struct file *filp)
{
	u32	gpio_reg;

	printk( "call minor0_open\n" );
	// Output(GPIO 17) --> SEL1, 21~23 bit                                  
	gpio_reg = readl( __io_address( GPFSEL1_ADDR ));                        
	gpio_reg &= ~(0x07 << (3*7)); // 21~23 bit     
	gpio_reg |= (0x01 << (3*7));	// output set 001
	/* IAMROOT-12CD (2016-07-02):
	 * --------------------------
	 * GPFSEL1_ADDR = 0x3F200004
	 * (*0x3F200004) = gpio_reg;
	 */
	writel( gpio_reg, __io_address( GPFSEL1_ADDR ));                        
	return 0;
}

ssize_t minor0_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char status;    
	get_user( status, (char *) buf ); 
	if(status == 0)
		writel( (1 << 17), __io_address( GPCLR0_ADDR ));
	else
		writel( (1 << 17), __io_address( GPSET0_ADDR ));

	return 1;
}

int minor0_release (struct inode *inode, struct file *filp)
{
	printk( "call minor0_release\n" );    
	return 0;
}

int minor1_open (struct inode *inode, struct file *filp)
{
	u32	gpio_reg;
	printk( "call minor1_open\n" );

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

	return 0;
}

ssize_t minor1_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	u32		status;

	status = readl( __io_address( GPLEV0_ADDR ));         
	if((status & (1 << 27)) == 0)
		put_user(0, &buf[0]);
	else
		put_user(1, &buf[0]);
	return 1;
}

int minor1_release (struct inode *inode, struct file *filp)
{
	printk( "call minor1_release\n" );
	return 0;
}

struct file_operations minor0_fops =
{
	.owner    = THIS_MODULE,
	.write    = minor0_write,
	.open     = minor0_open,
	.release  = minor0_release,
};

struct file_operations minor1_fops =
{
	.owner    = THIS_MODULE,
	.read     = minor1_read,
	.open     = minor1_open,
	.release  = minor1_release,
};

int minor_open (struct inode *inode, struct file *filp)
{
	printk( "call minor_open\n" );
	switch (MINOR(inode->i_rdev)) {
		case 1: filp->f_op = &minor0_fops; break;
		case 2: filp->f_op = &minor1_fops; break;
		default : return -ENXIO;
	}

	if (filp->f_op && filp->f_op->open)
		return filp->f_op->open(inode,filp);

	return 0;
}

struct file_operations minor_fops =
{
	.owner    = THIS_MODULE,
	.open     = minor_open,     
};

int minor_init(void)
{
	int result;

	result = register_chrdev( MINOR_DEV_MAJOR, MINOR_DEV_NAME, &minor_fops);
	if (result < 0) return result;

	return 0;
}

void minor_exit(void)
{
	unregister_chrdev( MINOR_DEV_MAJOR, MINOR_DEV_NAME );
}

module_init(minor_init);
module_exit(minor_exit);

MODULE_LICENSE("Dual BSD/GPL");
