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

#define   VRD_DEV_NAME            "vrd"
#define   VRD_DEV_MAJOR            240

#define   VRD_MAX_DEVICES          2

#define   MAJOR_NR                 VRD_DEV_MAJOR

#include <linux/blk.h>
#include <linux/blkpg.h>

#define   VRD_SECTOR_SIZE         512

#define   VRD_SIZE                (4*1024*1024)
#define   VRD_SIZE_KB             (VRD_SIZE/1024)
#define   VRD_SECTOR_TOTAL        (VRD_SIZE/VRD_SECTOR_SIZE)
#define   VRD_AHEAD               2

static char          *vdisk         [VRD_MAX_DEVICES] = {NULL,};
static int            vrd_size      [VRD_MAX_DEVICES] = { 0, };

static int vrd_make_request(request_queue_t * q, int rw, struct buffer_head *sbh)
{
    unsigned int  minor;
    char *pData;

    minor = MINOR(sbh->b_rdev);

    if( minor >= VRD_MAX_DEVICES ) goto fail;
    if( ( (sbh->b_rsector*VRD_SECTOR_SIZE) + sbh->b_size ) >= VRD_SIZE ) goto fail;  

    pData   = vdisk[minor] + (sbh->b_rsector*VRD_SECTOR_SIZE);
        
    switch(rw) 
    {
    case READA :         
    case READ  : memcpy(sbh->b_data, pData, sbh->b_size); 
                 break; 
    case WRITE : refile_buffer(sbh);
                 memcpy(pData, sbh->b_data, sbh->b_size); 
                 mark_buffer_uptodate(sbh, 1);
                 break;
    default    : goto fail;
    }
    sbh->b_end_io(sbh,1);
    return 0;
fail:
    buffer_IO_error(sbh);
    return 0;
} 

int vrd_open(struct inode *inode, struct file *filp)
{
    MOD_INC_USE_COUNT;
    return 0;
}

int vrd_release (struct inode *inode, struct file *filp)
{
    MOD_DEC_USE_COUNT;
    return 0;
}

int vrd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err, size;


    switch(cmd) 
    {

      case BLKGETSIZE:
        err = ! access_ok (VERIFY_WRITE, arg, sizeof(long));
        if (err) return -EFAULT;
        size = VRD_SECTOR_TOTAL;
	if (copy_to_user((long *) arg, &size, sizeof (long)))  return -EFAULT;
        return 0;

      default:
        return blk_ioctl(inode->i_rdev, cmd, arg);
    }

    return -ENOTTY; 
}

static struct block_device_operations vrd_fops =
{
    open               : vrd_open,
    release            : vrd_release,  
    ioctl              : vrd_ioctl,
};

int init_module(void)
{
    int lp;
    
    register_blkdev(MAJOR_NR, VRD_DEV_NAME, &vrd_fops);

    blk_queue_make_request(BLK_DEFAULT_QUEUE(MAJOR_NR), &vrd_make_request);

    read_ahead[MAJOR_NR]   = VRD_AHEAD;

    for( lp = 0; lp < VRD_MAX_DEVICES; lp++ )
        vrd_size[lp] = VRD_SIZE_KB;
    blk_size[MAJOR_NR] = vrd_size;
    
    vdisk[0] = vmalloc( VRD_SIZE );
    vdisk[1] = vmalloc( VRD_SIZE );
    
    for( lp = 0; lp < VRD_MAX_DEVICES; lp++ )
    {
	register_disk( NULL,
	               MKDEV(MAJOR_NR,lp),
		       1,
                       &vrd_fops,
		       VRD_SECTOR_TOTAL);
    }
    
    return 0;
}

void cleanup_module(void)
{
    int lp;
       
    unregister_blkdev( MAJOR_NR, VRD_DEV_NAME );
    
    vfree( vdisk[0]);
    vfree( vdisk[1]);

    read_ahead  [MAJOR_NR] = 0;
    blksize_size[MAJOR_NR] = NULL;
    blk_size    [MAJOR_NR] = NULL;
}
