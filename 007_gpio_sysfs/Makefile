obj-m := gpio_sys.o
gpio-objs += gpio_sys.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabi-
KERNEL_DIR=/home/jhkim/project/udemy/device_driver/workspace/custom_driver_4/source/linux

all: 
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(PWD) clean
copy-dtb:
	scp $(KERNEL_DIR)/arch/arm/boot/dts/am335x-boneblack.dtb debian@192.168.7.2:/home/debian
copy-drv:
	scp $(PWD)/*.ko debian@192.168.7.2:/home/debian
