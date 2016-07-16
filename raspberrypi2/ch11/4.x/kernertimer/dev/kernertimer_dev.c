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

#include <linux/time.h>
#include <linux/timer.h>

#define   KERNELTIMER_WRITE_ADDR           0x0378
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

#define   TIME_STEP                       (2*HZ/10)

typedef struct {
	struct timer_list  timer;
	unsigned long      led;
} __attribute__ ((packed)) KERNEL_TIMER_MANAGER;

static KERNEL_TIMER_MANAGER *ptrmng = NULL;

void kerneltimer_timeover(unsigned long arg );

void kerneltimer_registertimer( KERNEL_TIMER_MANAGER *pdata, unsigned long timeover )
{
	init_timer( &(pdata->timer) );
	pdata->timer.expires  = get_jiffies_64() + timeover;
	pdata->timer.data     = (unsigned long) pdata      ;
	pdata->timer.function = kerneltimer_timeover       ;
	add_timer( &(pdata->timer) );
}

void kerneltimer_timeover(unsigned long arg )
{
	KERNEL_TIMER_MANAGER *pdata = NULL;

	if( arg ) {
		pdata = ( KERNEL_TIMER_MANAGER * ) arg;

		if( pdata->led & 0xff == 0)
			writel( (1 << 17), __io_address( GPCLR0_ADDR ));
		else
			writel( (1 << 17), __io_address( GPSET0_ADDR ));

		pdata->led = ~(pdata->led);

		kerneltimer_registertimer( pdata , TIME_STEP );
	}
}

int kerneltimer_init(void)
{
	u32     gpio_reg;
	printk("B4 GPFSEL1: %08x\n", readl( __io_address( GPFSEL1_ADDR )));
	printk("B4 GPFSEL2: %08x\n", readl( __io_address( GPFSEL2_ADDR )));

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

	ptrmng = kmalloc( sizeof( KERNEL_TIMER_MANAGER ), GFP_KERNEL );
	if( ptrmng == NULL ) return -ENOMEM;

	memset( ptrmng, 0, sizeof( KERNEL_TIMER_MANAGER ) );

	ptrmng->led = 0;
	kerneltimer_registertimer( ptrmng, TIME_STEP );

	return 0;
}

void kerneltimer_exit(void)
{
	if( ptrmng != NULL ) {
		del_timer( &(ptrmng->timer) );
		kfree( ptrmng );
	}
	writel( (1 << 17), __io_address( GPCLR0_ADDR ));
}

module_init(kerneltimer_init);
module_exit(kerneltimer_exit);

MODULE_LICENSE("Dual BSD/GPL");

