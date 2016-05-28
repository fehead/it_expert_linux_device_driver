#define EXPORT_SYMTAB

#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/vmalloc.h>

#define   VHDD_MAX_DEVICE           2
#define   VHDD_DEVICE_SIZE          (4*1024*1024)

char *vdisk[VHDD_MAX_DEVICE] = {NULL,};

int init_module(void)
{
    int result;

    vdisk[0] = vmalloc(VHDD_DEVICE_SIZE);
    vdisk[1] = vmalloc(VHDD_DEVICE_SIZE);
    
    memset( vdisk[0], 0, VHDD_DEVICE_SIZE );
    memset( vdisk[1], 0, VHDD_DEVICE_SIZE );

    return 0;
}

void cleanup_module(void)
{
    vfree( vdisk[0]);
    vfree( vdisk[1]);
}

EXPORT_SYMBOL_NOVERS(vdisk);
