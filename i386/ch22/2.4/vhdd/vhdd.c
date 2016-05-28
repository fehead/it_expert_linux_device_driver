#define MODULE
#include <linux/module.h>
#include <linux/kernel.h>      

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/vmalloc.h>
#include <linux/hdreg.h>  
#include <asm/uaccess.h>

#define   VHDD_SECTOR_SIZE         512
#define   VHDD_SECTORS             16
#define   VHDD_HEADS               2
#define   VHDD_CYLINDERS           256

#define   VHDD_DEV_NAME            "vhdd"
#define   VHDD_DEV_MAJOR            240
#define   VHDD_SHIFT                4     
#define   VHDD_MAX_DEVICE           2

#define   VHDD_MAX_DEVICES          (VHDD_MAX_DEVICE<<VHDD_SHIFT)

#define   MAJOR_NR                  VHDD_DEV_MAJOR
#define   DEVICE_NAME               VHDD_DEV_NAME
#define   DEVICE_NR(device)         (MINOR(device)>>VHDD_SHIFT)
#define   DEVICE_REQUEST            vhdd_request

#include <linux/blk.h>
#include <linux/blkpg.h>

#define   VHDD_SIZE                (VHDD_SECTOR_SIZE*VHDD_SECTORS*VHDD_HEADS*VHDD_CYLINDERS)
#define   VHDD_SIZE_KB             (VHDD_SIZE/1024)

#define   VHDD_SECTOR_TOTAL        (VHDD_SECTORS*VHDD_HEADS*VHDD_CYLINDERS)

#define   VHDD_AHEAD               2

extern char          *vdisk          [VHDD_MAX_DEVICE]; 
static int            vhdd_size      [VHDD_MAX_DEVICES] = { 0, };
struct hd_struct      vhdd_partitions[VHDD_MAX_DEVICES] = { 0, };
struct timer_list     vhdd_timer; 
static int            vhdd_busy = 0;

extern struct gendisk vhdd_gendisk;

void vhdd_request(request_queue_t *q)
{
    int vdisk_num;
    int size, minor;
    char *pData;
    
    if( vhdd_busy ) return;
    
    while(1) 
    {
        if( QUEUE_EMPTY || (CURRENT->rq_status == RQ_INACTIVE) ) return;
        INIT_REQUEST;
        
        vdisk_num = DEVICE_NR(CURRENT->rq_dev);
        if( vdisk_num >= VHDD_MAX_DEVICE ) 
        {
            end_request(0);
            continue;
        }

        minor   = MINOR(CURRENT->rq_dev);

        if (CURRENT->sector + CURRENT->current_nr_sectors > vhdd_partitions[minor].nr_sects) 
        {
            end_request(0);
            continue;
        }

        pData   = vdisk[vdisk_num]
                + (vhdd_partitions[minor].start_sect + CURRENT->sector)*VHDD_SECTOR_SIZE;
        
        size    = CURRENT->current_nr_sectors*VHDD_SECTOR_SIZE; 
        
        switch(CURRENT->cmd) 
        {
        case READ  : memcpy(CURRENT->buffer, pData, size); break;
        case WRITE : memcpy(pData, CURRENT->buffer, size); break;
        default    : end_request(0);                       continue;
        }

        vhdd_timer.expires = jiffies + 2;
        add_timer(&vhdd_timer);
        vhdd_busy = 1;
        return;

    }
}

void vhdd_interrupt( unsigned long data )
{

    end_request(1);         
    vhdd_busy = 0;
    vhdd_request(NULL);
   
}


int vhdd_open(struct inode *inode, struct file *filp)
{
    MOD_INC_USE_COUNT;
    return 0;
}

int vhdd_release (struct inode *inode, struct file *filp)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

int vhdd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err, size;
    struct hd_geometry geo;

    switch(cmd) 
    {

      case BLKGETSIZE:
        err = ! access_ok (VERIFY_WRITE, arg, sizeof(long));
        if (err) return -EFAULT;
        size = vhdd_gendisk.part[MINOR(inode->i_rdev)].nr_sects;
	if (copy_to_user((long *) arg, &size, sizeof (long)))
	    return -EFAULT;
        return 0;

      case BLKFLSBUF: 
        if (! capable(CAP_SYS_RAWIO)) return -EACCES;
        fsync_dev(inode->i_rdev);
        invalidate_buffers(inode->i_rdev);
        return 0;

      case BLKRAGET: 
        err = ! access_ok(VERIFY_WRITE, arg, sizeof(long));
        if (err) return -EFAULT;
        put_user(read_ahead[MAJOR(inode->i_rdev)],(long *) arg);
        return 0;

      case BLKRASET: 
        if (!capable(CAP_SYS_RAWIO)) return -EACCES;
        if (arg > 0xff) return -EINVAL; 
        read_ahead[MAJOR(inode->i_rdev)] = arg;
        return 0;

      case BLKRRPART: 
        return 0;

      case HDIO_GETGEO:
        err = ! access_ok(VERIFY_WRITE, arg, sizeof(geo));
        if (err) return -EFAULT;

        geo.cylinders = VHDD_CYLINDERS;
	geo.heads     = VHDD_HEADS;
	geo.sectors   = VHDD_SECTORS;
	geo.start     = 4;
	
	if (copy_to_user((void *) arg, &geo, sizeof(geo)))
	    return -EFAULT;
        return 0;

      default:
        return blk_ioctl(inode->i_rdev, cmd, arg);
    }

    return -ENOTTY; 
}

static struct block_device_operations vhdd_fops =
{
    open               : vhdd_open,
    release            : vhdd_release,  
    ioctl              : vhdd_ioctl,
};

struct gendisk vhdd_gendisk = 
{
    major:              MAJOR_NR,       
    major_name:         VHDD_DEV_NAME,
    minor_shift:        VHDD_SHIFT,
    max_p:              1<<VHDD_SHIFT,
    fops:               &vhdd_fops,
};

int init_module(void)
{
    int lp;
    
    request_queue_t * q; 

    init_timer( &(vhdd_timer) );
    vhdd_timer.function = vhdd_interrupt;

    register_blkdev(MAJOR_NR, VHDD_DEV_NAME, &vhdd_fops);

    q = BLK_DEFAULT_QUEUE(MAJOR_NR);
    blk_init_queue(q, DEVICE_REQUEST );

    read_ahead[MAJOR_NR]   = VHDD_AHEAD;

    for( lp = 0; lp < VHDD_MAX_DEVICE; lp++ )
        vhdd_size[lp<<VHDD_SHIFT] = VHDD_SIZE_KB;
    blk_size[MAJOR_NR] = vhdd_size;
    
    vhdd_gendisk.sizes = vhdd_size;

    for( lp = 0; lp < VHDD_MAX_DEVICE; lp++ )
    {
        vhdd_partitions[lp<<VHDD_SHIFT].nr_sects = VHDD_SECTOR_TOTAL;
    }    

    vhdd_gendisk.part    = vhdd_partitions;
    vhdd_gendisk.nr_real = VHDD_MAX_DEVICE;
    add_gendisk(&vhdd_gendisk);

    for( lp = 0; lp < VHDD_MAX_DEVICE; lp++ )
    {
	register_disk(&vhdd_gendisk,
	               MKDEV(MAJOR_NR,lp<<VHDD_SHIFT),
		       1<<VHDD_SHIFT,
                       &vhdd_fops,
		       VHDD_SECTOR_TOTAL);
    }

    return 0;

}

void cleanup_module(void)
{
    int lp;
       
    for(lp = 0; lp < VHDD_MAX_DEVICES; lp++)
        fsync_dev(MKDEV(MAJOR_NR, lp)); 
        
    blk_cleanup_queue(BLK_DEFAULT_QUEUE(MAJOR_NR));

    read_ahead  [MAJOR_NR] = 0;
    blksize_size[MAJOR_NR] = NULL;
    blk_size    [MAJOR_NR] = NULL;

    del_gendisk(&vhdd_gendisk);
    unregister_blkdev( MAJOR_NR, VHDD_DEV_NAME );
    
}
