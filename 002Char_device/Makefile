obj-m := main.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabi-
KERNEL_DIR=/home/jhkim/project/udemy/device_driver/workspace/custom_driver_4/source/linux

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) clean
