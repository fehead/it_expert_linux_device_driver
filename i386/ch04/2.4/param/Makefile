KERNELDIR = /lib/modules/$(shell uname -r)/build
CFLAGS = -D__KERNEL__ -DMODULE -I$(KERNELDIR)/include -O

all: test.o

clean:
	rm -rf *.o
