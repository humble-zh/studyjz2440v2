KERN_DIR = /jz2440v2/linux-2.6.22.6

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf Module.symvers

obj-m	+= frame0_drv.o
