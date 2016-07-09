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

#include "ioctl_test.h"

#define   IOCTLTEST_DEV_NAME            "ioctldev"
#define   IOCTLTEST_DEV_MAJOR            240

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

int ioctltest_open (struct inode *inode, struct file *filp)
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

int ioctltest_release (struct inode *inode, struct file *filp)
{
    return 0;
}

long ioctltest_ioctl (struct file *filp, unsigned int cmd, unsigned long arg) {

	ioctl_test_info   ctrl_info;
	int               err, size;
	int               loop;
	u32		gpio_reg;

	if( _IOC_TYPE( cmd ) != IOCTLTEST_MAGIC ) return -EINVAL;
	if( _IOC_NR( cmd )   >= IOCTLTEST_MAXNR ) return -EINVAL;

	size = _IOC_SIZE( cmd );

	if( size ) {
		err = 0;
		if( _IOC_DIR( cmd ) & _IOC_READ  )
			err = !access_ok( VERIFY_WRITE, (void *) arg, size );
		else if( _IOC_DIR( cmd ) & _IOC_WRITE )
			err = !access_ok( VERIFY_READ , (void *) arg, size );

		if( err )
			return err;
	}

	switch( cmd ) {
	case IOCTLTEST_LEDOFF     :
		writel( (1 << 17), __io_address( GPCLR0_ADDR ));
		break;

	case IOCTLTEST_LEDON      :
		writel( (1 << 17), __io_address( GPSET0_ADDR ));
		break;

	case IOCTLTEST_GETSTATE   :
		gpio_reg = readl( __io_address( GPLEV0_ADDR ));
		return (gpio_reg & (1 << 27)) == 0 ? 0 : 1;

	case IOCTLTEST_READ       :
		gpio_reg = readl( __io_address( GPLEV0_ADDR ));
		ctrl_info.buff[0] =  (gpio_reg & (1 << 27)) == 0 ? 0 : 1;
		ctrl_info.size = 1;
		if (copy_to_user ( (void *) arg, (const void *) &ctrl_info, (unsigned long ) size ))
			return -EINVAL;
		break;

	case IOCTLTEST_WRITE      :
		if(copy_from_user ( (void *)&ctrl_info, (const void *) arg, size ))
			return -EINVAL;
		for( loop = 0; loop < ctrl_info.size; loop++ ) {
			if(ctrl_info.buff[loop] == 0)
				writel( (1 << 17), __io_address( GPCLR0_ADDR ));
			else
				writel( (1 << 17), __io_address( GPSET0_ADDR ));
		}

		break;

	case IOCTLTEST_WRITE_READ :
		if(copy_from_user ( (void *)&ctrl_info, (const void *) arg, size ))
			return -EINVAL;
		for( loop = 0; loop < ctrl_info.size; loop++ ) {
			if(ctrl_info.buff[loop] == 0)
				writel( (1 << 17), __io_address( GPCLR0_ADDR ));
			else
				writel( (1 << 17), __io_address( GPSET0_ADDR ));
		}

		gpio_reg = readl( __io_address( GPLEV0_ADDR ));
		ctrl_info.buff[0] =  (gpio_reg & (1 << 27)) == 0 ? 0 : 1;
		ctrl_info.size = 1;
		if(copy_to_user ( (void *) arg, (const void *) &ctrl_info, (unsigned long ) size ))
			return -EINVAL;
		break;

	}

	return 0;
}

struct file_operations ioctltest_fops =  {
	.owner    = THIS_MODULE,
	.unlocked_ioctl    = ioctltest_ioctl,
	.open     = ioctltest_open,
	.release  = ioctltest_release,
};

int ioctltest_init(void)
{
	int result;

	result = register_chrdev( IOCTLTEST_DEV_MAJOR, IOCTLTEST_DEV_NAME, &ioctltest_fops);
	if (result < 0) return result;

	return 0;
}

void ioctltest_exit(void)
{
	unregister_chrdev( IOCTLTEST_DEV_MAJOR, IOCTLTEST_DEV_NAME );
}

module_init(ioctltest_init);
module_exit(ioctltest_exit);

MODULE_LICENSE("Dual BSD/GPL");

