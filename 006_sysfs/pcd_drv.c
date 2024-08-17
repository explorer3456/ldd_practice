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
#include "platform.h"
#include "pcd_drv_sys.h"
#include <linux/of_device.h>


struct pcd_vdata pcd_vdata_list[3] = {
	[0] = {
		.version = CONFIG_V10_IDX,
		.supported_feature = 11111,
		.additional_action = 0x11,
	},
	[1] = {
		.version = CONFIG_V20_IDX,
		.supported_feature = 222,
		.additional_action = 0x22,
	},
	[2] = {
		.version = CONFIG_V30_IDX,
		.supported_feature = 333,
		.additional_action = 0x333,
	},
};

const struct of_device_id of_pcd_match_table[] = {
	{
		.compatible = "pcd_plat_dev-v1.0",
		.data = &pcd_vdata_list[0],
	},
	{
		.compatible = "pcd_plat_dev-v2.0",
		.data = &pcd_vdata_list[1],
	},
	{
		.compatible = "pcd_plat_dev-v3.0",
		.data = &pcd_vdata_list[2],
	},
	{}
};

struct pcdrv_private_data pcdrv_priv = {
	.total_devices = 0,
};
struct pcdev_private_data pcdev_priv[NUM_OF_DEVICES];


struct file_operations pcd_fops = {
	.owner = THIS_MODULE,
	.open = pcd_open,
	.release = pcd_release,
	.read = pcd_read,
	.write = pcd_write,
	.llseek = pcd_llseek,
};

ssize_t pcd_drv_max_size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pcdev_private_data * priv_ptr;
	int bytes;

	priv_ptr = dev_get_drvdata(dev); // we stored private data to dev->driver_data in probe function.

	bytes = scnprintf(buf, sizeof(int), "%u\n", priv_ptr->pdata.size );
	
	dev_info(dev, "max size show: %s\n", buf);

	return bytes;
}

ssize_t pcd_drv_max_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pcdev_private_data * priv_ptr;
	long resize;
	int ret;

	ret = 0;

	priv_ptr = dev_get_drvdata(dev);

	ret = kstrtol( buf, 0, &resize);
	if (ret != 0) {
		dev_err( dev, "error store: %d\n", ret);
		goto err_store;
	}
	dev_info(dev, "get resize data: %ld\n", resize);

	priv_ptr->buffer = krealloc( priv_ptr->buffer, resize, GFP_KERNEL);
	if (priv_ptr->buffer == NULL) {
		dev_err(dev, "krealloc is failed\n");
		ret = -ENOMEM;
		goto err_store;
	}

	priv_ptr->pdata.size = resize;

	return count;
err_store:
	return ret;
}

struct device_attribute pcd_drv_attr_max_size = {
	.attr = {
		.name = "max_size",
		.mode =  (S_IRUGO | S_IWUSR),
	},
	.show = pcd_drv_max_size_show,
	.store = pcd_drv_max_size_store,
};

// using macro for device attributes.
ssize_t serial_num_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pcdev_private_data * priv_ptr;
	int bytes;

	priv_ptr = dev_get_drvdata(dev); // we stored private data to dev->driver_data in probe function.

	bytes = scnprintf(buf, MAX_SERIAL_LENGTH, "%s\n", priv_ptr->pdata.serial_number );
	
	dev_info(dev, "serial num show: %s\n", buf);

	return bytes;
}

static DEVICE_ATTR_RO( serial_num );

static struct pcdev_platform_data * pcd_parse_dt(struct device * dev)
{
	int ret;
	struct pcdev_platform_data * pcd_plat_ptr;
	u32 out_value;
	const char * dt_string;

	if (dev->of_node == NULL)
		return NULL;

	dev_info( dev, "parse dt go\n");

	pcd_plat_ptr = devm_kzalloc( dev, sizeof(struct pcdev_platform_data), GFP_KERNEL );
	if (pcd_plat_ptr == NULL) {
		dev_err( dev, "kernel memory allocation is failed\n");
		ret = -ENOMEM;
		goto dt_fail_out;
	}

	dt_string = devm_kzalloc ( dev, sizeof(32) ,GFP_KERNEL);
	if (dt_string == NULL) {
		dev_err( dev, "kernel memory allocation failed \n");
		ret = -ENOMEM;
		goto dt_fail_out;
	}

	// parse all the platform data from device tree. 
	ret = of_property_read_u32( dev->of_node, "udemy,buf-size", &out_value);
	if (ret != 0 ) { 
		dev_err( dev, "device tree parsing failed: %d\n", ret);
		goto dt_fail_out;
	} else {
		pcd_plat_ptr->size = out_value;
	}

	ret = of_property_read_string_index( dev->of_node, "udemy,serial-num", 0, \
			&pcd_plat_ptr->serial_number );
	if (ret != 0 ) { 
		dev_err( dev, "device tree parsing failed: %d\n", ret);
		goto dt_fail_out;
	}

	ret = of_property_read_string_index( dev->of_node, "udemy,permission", 0, \
			&dt_string );
	if (ret != 0 ) { 
		dev_err( dev, "device tree parsing failed: %d\n", ret);
		goto dt_fail_out;
	} else {
		if (strcmp(dt_string, "RDONLY")) {
			pcd_plat_ptr->perm = PERM_READ_ONLY;
		}
		if (strcmp(dt_string, "RDWR")) {
			pcd_plat_ptr->perm = PERM_READ_WRITE;
		}
		if (strcmp(dt_string, "WRONLY")) {
			pcd_plat_ptr->perm = PERM_WRITE_ONLY;
		}
	}

	return pcd_plat_ptr;

dt_fail_out:
	return ERR_PTR(ret);
}

	// parse dt and update platform data.


static int pcd_probe(struct platform_device * pcdev) 
{
	int ret;

	struct pcdev_private_data * pcd_priv_ptr;
	struct pcdev_platform_data * pcd_plat_ptr; // platform device information. we need this.
	const struct pcd_vdata * pcd_vdata_ptr;
	const struct of_device_id * pcd_dev_id_ptr;

	pr_info("\n");

	// allocate device private data since we found devices
	pcd_priv_ptr = devm_kzalloc( &pcdev->dev, sizeof(struct pcdev_private_data), GFP_KERNEL);
	if (pcd_priv_ptr == NULL) {
		pr_err("kernel memory allocation is failed\n");
		ret = -ENOMEM;
		goto out;
	}
	
	// lets decide whether the probe is called due to device tree or not more precisely.
	// of_match_ptr returns NULL if config_of is not defined, otherwise, it returns the argument. 
	// so it can be used to determine whether config_of is on or off.
	pcd_dev_id_ptr = of_match_device( of_match_ptr(pcdev->dev.driver->of_match_table), &pcdev->dev);

	if (pcd_dev_id_ptr != NULL) {
		// parse platform data.
		pcd_plat_ptr = pcd_parse_dt( &pcdev->dev );

		if (IS_ERR(pcd_plat_ptr) ) {
			dev_err( &pcdev->dev, "dt parse failed\n");
			ret = PTR_ERR(pcd_plat_ptr);
			goto out;
		}

		// parse driver config data.
		pcd_vdata_ptr = of_device_get_match_data( &pcdev->dev );

		if (pcd_vdata_ptr == NULL) {
			dev_err( &pcdev->dev, " There is no matched driver data\n");
			ret = -ENODATA;
			goto out;
		}
	} else {
		pcd_plat_ptr = pcdev->dev.platform_data;

		if (pcdev->id_entry != NULL) {
			pcd_vdata_ptr = &pcd_vdata_list[pcdev->id_entry->driver_data];
		} else {
			ret = -ENODATA;
			goto out;
		}
	}

	pcd_priv_ptr->pdata.size = pcd_plat_ptr->size;
	// sprintf(pcd_priv_ptr->pdata.serial_number, pcd_plat_ptr->serial_number);
	pcd_priv_ptr->pdata.serial_number = pcd_plat_ptr->serial_number;
	pcd_priv_ptr->pdata.perm = pcd_plat_ptr->perm;

	// allocate user interfaces variables.
	pcd_priv_ptr->buffer = devm_kzalloc( &pcdev->dev, (pcd_priv_ptr->pdata.size) * sizeof(pcd_priv_ptr->pdata.size), GFP_KERNEL);

	if (pcd_priv_ptr->buffer == NULL) {
		pr_err("kernel memory allocation is failed\n");
		ret = -ENOMEM;
		goto dev_data_free;
	}

	// pcdev_priv[id].dev_num = pcdrv_priv.dev_num_base + id;
	pcd_priv_ptr->dev_num = pcdrv_priv.dev_num_base + pcdrv_priv.total_devices;


	cdev_init( &(pcd_priv_ptr->cdev), &pcd_fops);
	pcd_priv_ptr->cdev.owner = THIS_MODULE;

	ret = cdev_add( &pcd_priv_ptr->cdev, pcd_priv_ptr->dev_num, 1);
	if (ret < 0 ) {
		pr_err(" cdev add failed: %d\n", ret);
		goto cdev_add_failed;
	}

	pcdrv_priv.device_pcd = device_create( pcdrv_priv.class_pcd, &pcdev->dev, pcd_priv_ptr->dev_num , NULL, \
			"pcd-dev-create-%d", pcdrv_priv.total_devices);

	if (IS_ERR(pcdrv_priv.device_pcd)) {
		ret = PTR_ERR(pcdrv_priv.device_pcd);
		pr_err("device create failed: %d\n", ret);
		goto device_create_failed;
	}

	// since probe is successful, we want to save the device private data.
	// so that other file operations can access to private data.
	pcdev->dev.driver_data = pcd_priv_ptr;

	pr_info("device probed\n");

	dev_info( &pcdev->dev, "sysfs create of device attreibutes\n");

	ret = sysfs_create_file( &pcdev->dev.kobj, &pcd_drv_attr_max_size.attr );
	if (ret != 0) {
		dev_err( &pcdev->dev, "sysfs creation failed: %d\n", ret);
	}

	ret = sysfs_create_file( &pcdev->dev.kobj, &dev_attr_serial_num.attr );
	if (ret != 0) {
		dev_err( &pcdev->dev, "sysfs creation failed: %d\n", ret);
	}
	
	// print device information

	pr_info("devnumber: %08x, major: %d, minor: %d \n", pcd_priv_ptr->dev_num,  \
			MAJOR(pcd_priv_ptr->dev_num), MINOR(pcd_priv_ptr->dev_num));

	pr_info("--size: %d\n", pcd_priv_ptr->pdata.size);
	pr_info("--serial number: %s\n", pcd_priv_ptr->pdata.serial_number);
	pr_info("--permission: %d\n", pcd_priv_ptr->pdata.perm);

	pr_info("driver data == \n");
	pr_info("== device version: %d\n", pcd_vdata_ptr->version);
	pr_info(" supported feature: %d\n", pcd_vdata_ptr->supported_feature);
	pr_info(" additional operation: %d\n", pcd_vdata_ptr->additional_action);

	pcdrv_priv.total_devices++;

	return 0;

device_create_failed:
	cdev_del( &pcd_priv_ptr->cdev );
cdev_add_failed:
	// we dont need devm kfree since we are using devres API
	// devm_kfree ( &pcdev->dev, pcd_priv_ptr->buffer );
dev_data_free:
	// we dont need devm kfree since we are using devres API
	// devm_kfree( &pcdev->dev, pcd_priv_ptr );
out:
	return ret;
};

static int pcd_remove(struct platform_device *pcdev) 
{
	int ret;
	struct pcdev_private_data * pcd_priv_ptr;

	ret = 0;

	pcd_priv_ptr = pcdev->dev.driver_data;
	if (pcd_priv_ptr == NULL) {
		ret = -EFAULT; 
		pr_err("invalid private data: %d\n", ret);
		goto out;
	}

	sysfs_remove_file( &pcdev->dev.kobj, &pcd_drv_attr_max_size.attr);
	sysfs_remove_file( &pcdev->dev.kobj, &dev_attr_serial_num.attr);

	device_destroy( pcdrv_priv.class_pcd, pcd_priv_ptr->dev_num );
	cdev_del( &pcd_priv_ptr->cdev );

	// free the memory.
	// we dont need to free the memroy since we are using device resource management API.
	// kfree( pcd_priv_ptr->buffer);
	// kfree( pcd_priv_ptr );

	pr_info("device removed\n");

out:
	return ret;
};


const struct platform_device_id pcd_id_table[] = {
	[0] = { 
		.name = "pcd_plat_dev-v1.0",
		.driver_data = CONFIG_V10_IDX,
	},
	[1] = { 
		.name = "pcd_plat_dev-v2.0",
		.driver_data = CONFIG_V20_IDX,
	},
	[2] = {
		.name = "pcd_plat_dev",
		.driver_data = CONFIG_DEF_IDX,
	},
	{}
};


struct platform_driver pcd_plat_driver = {
	.probe = pcd_probe,
	.remove = pcd_remove,
	.id_table = pcd_id_table,
	.driver = {
		.name = "pcd_plat_driver",
		.of_match_table = of_pcd_match_table,
	},
};

static int __init pcd_module_init(void)
{
	int ret;

	ret = 0;

	pr_info("module init start\n");

	// alloc device number.
	ret = alloc_chrdev_region( &pcdrv_priv.dev_num_base, 0, NUM_OF_DEVICES, "pcd_device_num");
	if (ret < 0) {
		pr_err("alloc chrdev region failed: %d\n", ret);
		goto chrdev_failed;
	}

	// class_pcd creation.
	pcdrv_priv.class_pcd = class_create( THIS_MODULE, "pcd_class_pcd");
	if (IS_ERR(pcdrv_priv.class_pcd) ) {
		pr_err("class_pcd creation failed\n");
		ret = PTR_ERR(pcdrv_priv.class_pcd);
		goto class_failed;
	}

	ret = platform_driver_register( &pcd_plat_driver );
	if (ret < 0) {
		pr_err("module init failed\n");
		return ret;
	}

class_failed:
	unregister_chrdev_region( pcdrv_priv.dev_num_base, NUM_OF_DEVICES);
chrdev_failed:
	return ret;
}

static void __exit pcd_module_exit(void)
{
	pr_info("module exit start\n");
	platform_driver_unregister( &pcd_plat_driver );
	class_destroy( pcdrv_priv.class_pcd ); 
	unregister_chrdev_region( pcdrv_priv.dev_num_base, NUM_OF_DEVICES);
	pr_info("module exit done\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
