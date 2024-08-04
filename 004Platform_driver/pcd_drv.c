#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/linux/fs.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include "platform.h"

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

struct pcdrv_private_data pcdrv_priv;
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
	pr_info("\n");
	return 0;
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

static int pcd_probe(struct platform_device * pcdev) 
{
	int ret;
	int id;

	struct pcdev_platform_data * pcd_plat_ptr; // platform device information. we need this.

	pcd_plat_ptr = pcdev->dev.platform_data;

	id = pcdev->id;

	pcdev_priv[id].dev_num = pcdrv_priv.dev_num_base + id;

	pr_info("devnumber: %08x, major: %d, minor: %d \n", pcdev_priv[id].dev_num,  \
			MAJOR(pcdev_priv[id].dev_num), MINOR(pcdev_priv[id].dev_num));

	cdev_init( &(pcdev_priv[id].cdev), &pcd_fops);
	pcdev_priv[id].cdev.owner = THIS_MODULE;

	ret = cdev_add( &pcdev_priv[id].cdev, pcdev_priv[id].dev_num, 1);
	if (ret < 0 ) {
		pr_err(" cdev add failed: %d\n", ret);
		goto cdev_add_failed;
	}

	pcdrv_priv.device_pcd = device_create( pcdrv_priv.class_pcd, NULL, pcdev_priv[id].dev_num , NULL, \
			"pcd-dev-create-%d", id);

	if (IS_ERR(pcdrv_priv.device_pcd)) {
		ret = PTR_ERR(pcdrv_priv.device_pcd);
		pr_err("device create failed: %d\n", ret);
		goto device_create_failed;
	}

	pr_info("device probed\n");
	return 0;

device_create_failed:
	cdev_del( &pcdev_priv[id].cdev );
cdev_add_failed:
	return ret;
};

static int pcd_remove(struct platform_device *pcdev) 
{
	int id;

	id = pcdev->id;

	pr_err("remove id: %d\n", id);

	device_destroy( pcdrv_priv.class_pcd, pcdev_priv[id].dev_num );
	cdev_del( &pcdev_priv[id].cdev );
	pr_info("device removed\n");

	return 0;
};

struct platform_driver pcd_plat_driver = {
	.probe = pcd_probe,
	.remove = pcd_remove,
	.driver = {
		.name = "pcd_plat_dev",
	}
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
