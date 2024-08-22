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
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>

#undef pr_fmt
#define pr_fmt(fmt) "[GPIO_DRV][%s] : " fmt, __func__

struct gpio_desc {
        struct gpio_device      *gdev;
        unsigned long           flags;
/* flag symbols are bit numbers */
#define FLAG_REQUESTED  0
#define FLAG_IS_OUT     1
#define FLAG_EXPORT     2       /* protected by sysfs_lock */
#define FLAG_SYSFS      3       /* exported via /sys/class/gpio/control */
#define FLAG_ACTIVE_LOW 6       /* value has active low */
#define FLAG_OPEN_DRAIN 7       /* Gpio is open drain type */
#define FLAG_OPEN_SOURCE 8      /* Gpio is open source type */
#define FLAG_USED_AS_IRQ 9      /* GPIO is connected to an IRQ */
#define FLAG_IS_HOGGED  11      /* GPIO is hogged */
#define FLAG_TRANSITORY 12      /* GPIO may lose value in sleep or reset */
           
        /* Connection label */
        const char              *label;
        /* Name of the GPIO */
        const char              *name;
};  

struct gpio_device_private {
	const char * label;
	struct gpio_desc *gpio_desc;
};

struct gpio_driver_private {
	int total_devices;
	struct class * class;

	struct device **devices;
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

ssize_t direction_show(struct device * dev, struct device_attribute *attr, char *buf)
{
	struct gpio_device_private * dev_priv_ptr;
	struct gpio_desc * gpio_desc_ptr;
	int ret;
	int bytes;

	dev_priv_ptr = dev_get_drvdata((const struct device * )dev);
	gpio_desc_ptr = dev_priv_ptr->gpio_desc;

	ret = gpiod_get_direction( gpio_desc_ptr );

	if (ret == 0) {
		bytes = scnprintf(buf, sizeof(char)*4, "%s\n", "out");
		ret = bytes;
	} else if (ret == 1) {
		bytes = scnprintf(buf, sizeof(char)*3, "%s\n", "in");
		ret = bytes; 
	} else {
		dev_err( (const struct device *)dev, "cannot get gpio direction\n");
	}

	return ret;
}

ssize_t direction_store(struct device * dev, struct device_attribute *attr, const char *buf, size_t count) 
{
	struct gpio_device_private * dev_priv_ptr;
	struct gpio_desc * gpio_desc_ptr;
	int ret;

	dev_priv_ptr = dev_get_drvdata(dev);
	gpio_desc_ptr = dev_priv_ptr->gpio_desc;

	if ( strcmp(buf, "in") ) {
		ret = gpiod_direction_input( gpio_desc_ptr );
		if (ret != 0) {
			dev_err( dev, "input setting error\n");
			goto store_out;
		} else 
			ret = count;
	} else if ( strcmp(buf, "out") ) {
		ret = gpiod_direction_output( gpio_desc_ptr, 0);
		if (ret != 0) {
			dev_err( dev, "input setting error\n");
			goto store_out;
		} else 
			ret = count;
	} else {
		dev_err(dev, "unknown store argument\n");
		ret = -EINVAL;
	}
store_out:
	return ret;
}

struct device_attribute dev_attr_direction = {
	.attr = {
		.name = "direction",
		.mode = (S_IRUGO | S_IWUSR),
	},
	.show = direction_show,
	.store = direction_store,
};

int gpio_sys_probe(struct platform_device * pdev)
{
	int ret;
	struct gpio_device_private * dev_priv_ptr;
	int i;
	struct device_node * child;

	// since there are only few of data from gpio dt,, lets not use plat data.
	dev_info( &pdev->dev, "probe start\n");
	
	gpio_drv_priv.total_devices = of_get_child_count( pdev->dev.of_node );

	gpio_drv_priv.devices = devm_kzalloc( &pdev->dev, sizeof(struct device *) * (gpio_drv_priv.total_devices), \
			GFP_KERNEL);

	if (gpio_drv_priv.devices == NULL) {
		dev_err( &pdev->dev, "alloc failed\n");
		ret = -ENOMEM;
		goto out;
	}

	i = 0;

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

		ret = of_property_read_string( child, "udemy,label", \
				&(dev_priv_ptr->label) );
		if (ret != 0 ) { // property parsing failed
			scnprintf( dev_priv_ptr->label, 20, "unknown-%d", \
					i);
		}

		gpio_drv_priv.devices[i] = device_create( gpio_drv_priv.class, &pdev->dev, 0, NULL, \
				"gpio-dev-create-%d", i);


		// dev_priv_ptr->gpio_desc = devm_gpiod_get_from_of_node( &pdev->dev, child,
				// "udemy1-gpios", 0, GPIOD_ASIS, "lab_at");
		dev_priv_ptr->gpio_desc = devm_fwnode_get_gpiod_from_child( &pdev->dev, "udemy1", \
				&child->fwnode, GPIOD_ASIS, "labb");

		dev_info( &pdev->dev, "%d: label:%s, name:%s,\n", i, \
				dev_priv_ptr->gpio_desc->label, dev_priv_ptr->gpio_desc->name);

		dev_set_drvdata( gpio_drv_priv.devices[i], dev_priv_ptr);

		// configure default gpio direction.
		ret = gpiod_direction_output_raw( dev_priv_ptr->gpio_desc, 0);
		if (ret != 0 ) {
			dev_err( &pdev->dev, "gpio direction setting failed: %d\n", i);
			goto out;
		}

		// register sysfs.
		ret = sysfs_create_file( &(gpio_drv_priv.devices[i])->kobj, &dev_attr_direction.attr);
		if (ret != 0 ) {
			dev_err( &pdev->dev, "sysfs creation failed\n");
		}

		i++;
	}

	return 0;
out:
	return ret;
}

int gpio_sys_remove(struct platform_device * pdev)
{
	int i;
	
	dev_info( &pdev->dev, "remove \n");

	for (i=0; i < gpio_drv_priv.total_devices; i++) {
		device_unregister( gpio_drv_priv.devices[i] );
	}

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
