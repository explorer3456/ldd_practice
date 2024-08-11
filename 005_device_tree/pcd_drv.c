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
#include <linux/of_device.h>

struct pcdev_private_data
{
	struct pcdev_platform_data pdata;
	struct cdev cdev;
	dev_t dev_num;
	char * buffer;
};

struct pcdrv_private_data
{
	int total_devices;
	dev_t dev_num_base;
	struct class * class_pcd;
	struct device * device_pcd;
};

enum {
	CONFIG_V10_IDX = 0,
	CONFIG_V20_IDX,
	CONFIG_V30_IDX,
	CONFIG_DEF_IDX,
};

struct pcd_vdata {
	int version;
	int supported_feature;
	int additional_action;
};

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

bool check_permission(fmode_t file_perm, int dev_perm)
{
	bool ret;

	ret = false;

	if (dev_perm == PERM_READ_ONLY) {
		if ( ((file_perm & FMODE_READ) == FMODE_READ) && ((file_perm & FMODE_WRITE) != FMODE_WRITE) )
			ret = true;
	} else if (dev_perm == PERM_WRITE_ONLY) {
		if ( ((file_perm & FMODE_WRITE) == FMODE_WRITE) && ((file_perm & FMODE_READ) != FMODE_READ) )
			ret = true;
	} else 
		ret = true;

	if (!ret)
		pr_err("Permission denied\n");

	return ret;
}

#undef pr_fmt
#define pr_fmt(fmt) "[PCD_DRV][%s] : " fmt, __func__

loff_t pcd_llseek (struct file *filep, loff_t offset, int whence)
{
	pr_info("\n");
	return 0;
};

ssize_t pcd_read (struct file *filep, char __user *buf, size_t count, loff_t * f_pos)
{
	pr_info("\n");
	return 0;
};

ssize_t pcd_write (struct file *filep, const char __user *buf, size_t count, loff_t * f_pos)
{
	pr_info("\n");
	return 0;
};

int pcd_open (struct inode *inodep, struct file *filep)
{
	int ret;
	struct pcdev_private_data * pcd_priv_ptr;
	struct pcdev_platform_data * pdata;

	ret = 0;

	// get device private data.

	// container_of( ptr, type, member );
	pcd_priv_ptr = container_of(inodep->i_cdev, struct pcdev_private_data, cdev);

	pr_info("get pcd priv data\n");
	pr_info("--devnumber: %08x, major: %d, minor: %d \n", pcd_priv_ptr->dev_num,  \
			MAJOR(pcd_priv_ptr->dev_num), MINOR(pcd_priv_ptr->dev_num));

	pr_info("--size: %d\n", pcd_priv_ptr->pdata.size);
	pr_info("--serial number: %s\n", pcd_priv_ptr->pdata.serial_number);
	pr_info("--permission: %d\n", pcd_priv_ptr->pdata.perm);

	pdata = &pcd_priv_ptr->pdata;

	filep->private_data = pcd_priv_ptr;
// bool check_permission(fmode_t file_perm, int dev_perm)

	if (!check_permission( filep->f_mode, pcd_priv_ptr->pdata.perm )) {
		pr_err("Permission denied\n");
		ret = -EPERM;
		goto out;
	}

	pr_info("\n");
out:
	return ret;
};

int pcd_release (struct inode *inodep, struct file *filep)
{
	pr_info("\n");
	return 0;
};

struct file_operations pcd_fops = {
	.owner = THIS_MODULE,
	.open = pcd_open,
	.release = pcd_release,
	.read = pcd_read,
	.write = pcd_write,
	.llseek = pcd_llseek,
};

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

	pr_info("\n");

	// allocate device private data since we found devices
	pcd_priv_ptr = devm_kzalloc( &pcdev->dev, sizeof(struct pcdev_private_data), GFP_KERNEL);
	if (pcd_priv_ptr == NULL) {
		pr_err("kernel memory allocation is failed\n");
		ret = -ENOMEM;
		goto out;
	}

	// parse platform data.
	pcd_plat_ptr = pcd_parse_dt( &pcdev->dev );
	if (pcd_plat_ptr == NULL) { // of node is NULL.
		pcd_plat_ptr = pcdev->dev.platform_data;
	} else if (IS_ERR(pcd_plat_ptr) ) {
		dev_err( &pcdev->dev, "dt parse failed\n");
		ret = PTR_ERR(pcd_plat_ptr);
		goto out;
	}

	// get driver config data.
	if (pcdev->id_entry != NULL) {
		pcd_vdata_ptr = &pcd_vdata_list[pcdev->id_entry->driver_data];
	}else{
#if 0
		of_dev_id_ptr = of_match_node( pcdev->dev.driver->of_match_table, pcdev->dev.of_node);
		if (of_dev_id_ptr == NULL) {
			ret = -ENODATA;
			goto out;
		} else {
			pcd_vdata_ptr = (struct pcd_vdata *)of_dev_id_ptr->data;
		}
#else
		pcd_vdata_ptr = of_device_get_match_data( &pcdev->dev );

		if (pcd_vdata_ptr == NULL) {
			dev_err( &pcdev->dev, " There is no matched driver data\n");
			ret = -ENODATA;
			goto out;
		}
#endif
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

	pcdrv_priv.device_pcd = device_create( pcdrv_priv.class_pcd, NULL, pcd_priv_ptr->dev_num , NULL, \
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
