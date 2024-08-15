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

#undef pr_fmt
#define pr_fmt(fmt) "[PCD_DRV][%s] : " fmt, __func__

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

loff_t pcd_llseek (struct file *filep, loff_t offset, int whence);
ssize_t pcd_read (struct file *filep, char __user *buf, size_t count, loff_t * f_pos);
ssize_t pcd_write (struct file *filep, const char __user *buf, size_t count, loff_t * f_pos);
int pcd_open (struct inode *inodep, struct file *filep);
int pcd_release (struct inode *inodep, struct file *filep);
