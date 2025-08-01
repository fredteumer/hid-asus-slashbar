obj-m := hid-asus-slashbar-v2.o
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

# Keep old version for reference
old:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) hid-asus-slashbar.ko
