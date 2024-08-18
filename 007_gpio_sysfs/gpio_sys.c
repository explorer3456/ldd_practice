#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/linux/fs.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>

#undef pr_fmt
#define pr_fmt(fmt) "[GPIO_DRV][%s] : " fmt, __func__


struct gpio_device_private {
	char label[20];
};

struct gpio_driver_private {
	int total_devices;
	struct class * class;
	struct device * device;
};

struct gpio_driver_private gpio_drv_priv = {
	.total_devices = 0,
};

const struct of_device_id gpio_sys_match_id[] = {
	[0] = {
		.compatible = "udemy,gpio-drv",
	},
};

int gpio_sys_probe(struct platform_device * pdev)
{
	return 0;
}

int gpio_sys_remove(struct platform_device * pdev)
{
	return 0;
}

struct platform_driver gpio_sys_driver = {
	.probe = gpio_sys_probe,
	.remove = gpio_sys_remove,
	.driver = {
		.name = "gpio_sys_driver",
		.of_match_table = of_match_ptr( gpio_sys_match_id),
	},
};

static int __init gpio_module_init(void)
{
	int ret;

	ret = 0;

	pr_info("module_ init\n");

	gpio_drv_priv.class = class_create( THIS_MODULE, "gpio_sysfs_class");
	if ( IS_ERR(gpio_drv_priv.class) ) {
		pr_err("class creation failed\n");
		ret = PTR_ERR( gpio_drv_priv.class );
		goto class_failed;
	}

	ret = platform_driver_register( &gpio_sys_driver );
	if ( ret < 0 ) {
		pr_err("platform driver reg failed\n");
		return ret;
	}

	return ret;

class_failed:
	return ret;
}

static void __exit gpio_module_exit(void)
{
	pr_info("module exit start\n");
	platform_driver_unregister( &gpio_sys_driver );
	class_destroy( gpio_drv_priv.class );
	pr_info("module exit done\n");
}

module_init(gpio_module_init);
module_exit(gpio_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
