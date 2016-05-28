#ifndef _IOCTLTEST_H_
#define _IOCTLTEST_H_

#define IOCTLTEST_MAGIC    't'

typedef struct
{
	unsigned long size;
	unsigned char buff[128];
} __attribute__ ((packed)) ioctl_test_info;

#define IOCTLTEST_LEDOFF           _IO(  IOCTLTEST_MAGIC, 0 )
#define IOCTLTEST_LEDON            _IO(  IOCTLTEST_MAGIC, 1 )
#define IOCTLTEST_GETSTATE         _IO(  IOCTLTEST_MAGIC, 2 ) 

#define IOCTLTEST_READ             _IOR( IOCTLTEST_MAGIC, 3 , ioctl_test_info )
#define IOCTLTEST_WRITE            _IOW( IOCTLTEST_MAGIC, 4 , ioctl_test_info )
#define IOCTLTEST_WRITE_READ       _IOW( IOCTLTEST_MAGIC, 5 , ioctl_test_info )

#define IOCTLTEST_MAXNR                                   6
  
#endif // IOCTLTEST_H_

