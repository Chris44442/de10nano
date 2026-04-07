# Makefile
obj-m := hello.o

KDIR ?= /home/chris/repo/uboot_linux_buildroot/linux/

# Cross-compiler prefix (Buildroot toolchain)
CROSS_COMPILE ?= /home/chris/repo/uboot_linux_buildroot/toolchain/armv7-eabihf--glibc--stable-2020.08-1/bin/arm-linux-

all:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
