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
	pr_info("device probed\n");
	return 0;
};

static int pcd_remove(struct platform_device *pcdev) 
{
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

	pr_info("module init start\n");
	ret = platform_driver_register( &pcd_plat_driver );
	if (ret < 0) {
		pr_err("module init failed\n");
	} else {
		pr_info("module init done\n");
	}

	return ret;
}

static void __exit pcd_module_exit(void)
{
	pr_info("module exit start\n");
	platform_driver_unregister( &pcd_plat_driver );
	pr_info("module exit done\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
