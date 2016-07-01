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

#define   RDWR_DEV_NAME            "rdwrdev"
#define   RDWR_DEV_MAJOR            240      

#define   RDWR_WRITE_ADDR        0x0378      
#define   RDWR_READ_ADDR         0x0379      

#define	GPFSEL1_ADDR	(GPIO_BASE + 0x04)
#define	GPFSEL2_ADDR	(GPIO_BASE + 0x08)

#define GPSET0_ADDR	(GPIO_BASE + GPSET0)
#define GPCLR0_ADDR	(GPIO_BASE + GPCLR0)
#define GPLEV0_ADDR	(GPIO_BASE + GPLEV0)
#define	GPIO_LED_NR	7		/* GPIO 17 pin */
#define GPIO_LED_GR	0		/* GPIO Base Pin */

int rdwr_open (struct inode *inode, struct file *filp)
{
	u32	gpio_reg; 

	printk("B4 GPFSEL1: %08ux", readl( __io_address( GPFSEL1_ADDR )));
	printk("B4 GPFSEL2: %08ux", readl( __io_address( GPFSEL2_ADDR )));

	// Output(GPIO 17) --> SEL1, 21~23 bit
	gpio_reg = readl( __io_address( GPFSEL1_ADDR ));
	gpio_reg &= ~(0x07 << (3*(GPIO_LED_NR - GPIO_LED_GR)));	// 21~23 bit
	gpio_reg |= (0x01 << (3*(GPIO_LED_NR - GPIO_LED_GR)));
	writel( gpio_reg, __io_address( GPFSEL1_ADDR ));

	// Input(GPIO 27) --> SEL2, 21~23 bit
	gpio_reg = readl( __io_address( GPFSEL2_ADDR ));
	gpio_reg &= ~(0x07 << (3*(GPIO_LED_NR - GPIO_LED_GR)));
	gpio_reg |= (0x00 << (3*(GPIO_LED_NR - GPIO_LED_GR)));
	writel( gpio_reg, __io_address( GPFSEL1_ADDR ));

	printk("A4 GPFSEL1: %08ux", readl( __io_address( GPFSEL1_ADDR )));
	printk("A4 GPFSEL2: %08ux", readl( __io_address( GPFSEL2_ADDR )));

	return 0;
}

ssize_t rdwr_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char	tmp_buf;

	tmpbuf = readl( __io_address( GPLEV0 ) & (1 << 27)) == 0 ? 0 : 1;

	copy_to_user(buf, &tmp_buf, 1);

	return 1;
}

ssize_t rdwr_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char	tmp_buf;
	copy_from_user(&tmp_buf, buf, sizeof(tmp_buf));
	
	// Ctrl LED
	if(0 == tmp_buf)
		writel( (0x01 << 17), __io_address( GPCLR0_ADDR ));
	else
		writel( (0x01 << 17), __io_address( GPSET0_ADDR ));

	return 1;
}

int rdwr_release (struct inode *inode, struct file *filp)
{
    return 0;
}

struct file_operations rdwr_fops =
{
    .owner    = THIS_MODULE,
    .read     = rdwr_read,     
    .write    = rdwr_write,    
    .open     = rdwr_open,     
    .release  = rdwr_release,  
};

int rdwr_init(void)
{
    int result;

    printk("[gpio_open_close] rdwr_init() called\n");

    result = register_chrdev( RDWR_DEV_MAJOR, RDWR_DEV_NAME, &rdwr_fops);
    if (result < 0) return result;

    return 0;
}

void rdwr_exit(void)
{
    printk("[gpio_open_close] rdwr_exit() called\n");
    unregister_chrdev( RDWR_DEV_MAJOR, RDWR_DEV_NAME );
}

module_init(rdwr_init);
module_exit(rdwr_exit);

MODULE_LICENSE("Dual BSD/GPL");
