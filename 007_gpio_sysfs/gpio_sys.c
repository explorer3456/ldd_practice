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
	const char * label;
	dev_t dev_num;
	// gpio information shuold be in here. because, private data of device is used for fops, sysfs.. etc.
};

struct gpio_driver_private {
	int total_devices;
	struct class * class;
	struct device * device;
	dev_t dev_num_base;
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
	int ret;
	struct gpio_device_private * dev_priv_ptr;
	int i;
	struct device_node * child;

	// since there are only few of data from gpio dt,, lets not use plat data.
	dev_info( &pdev->dev, "probe start\n");

	// number of children.
	for_each_child_of_node( pdev->dev.of_node, child ) {

		dev_priv_ptr = devm_kzalloc( &pdev->dev, sizeof(struct gpio_device_private), GFP_KERNEL);
		if (dev_priv_ptr == NULL) {
			dev_err( &pdev->dev, "alloc failed\n");
			ret = -ENOMEM;
			goto out;
		}

		dev_priv_ptr->label = devm_kzalloc( &pdev->dev, 20, GFP_KERNEL);
		if (dev_priv_ptr->label == NULL) {
			dev_err( &pdev->dev, "alloc failed\n");
			ret = -ENOMEM;
			goto out;
		}

		ret = of_property_read_string_index( child, "udemy,label", 0, \
				&dev_priv_ptr->label );

		gpio_drv_priv.device = device_create( gpio_drv_priv.class, &pdev->dev, 0, NULL, \
				"gpio-dev-create-%d", gpio_drv_priv.total_devices);

		dev_set_drvdata( gpio_drv_priv.device, dev_priv_ptr);

		// need to learn gpio node parsing method.
		// need to implement sysfs.

	}

	return 0;
out:
	return ret;
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
