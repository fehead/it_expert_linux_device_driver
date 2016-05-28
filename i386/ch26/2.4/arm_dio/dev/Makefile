KERNELDIR = /var/data/project/ez-x5/kernel/linux
CFLAGS = -D__KERNEL__ -DMODULE -I$(KERNELDIR)/include -O

CC=armv5l-linux-gcc 

all: arm_dio_dev.o
	cp -a arm_dio_dev.o /nfsyyc/
clean:
	rm -rf *.o

