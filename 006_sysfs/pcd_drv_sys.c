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

ssize_t pcd_drv_max_size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pcdev_private_data * priv_ptr;
	int bytes;

	priv_ptr = dev_get_drvdata(dev); // we stored private data to dev->driver_data in probe function.

	bytes = scnprintf(buf, PAGE_SIZE, "%u\n", priv_ptr->pdata.size );
	
	dev_info(dev, "show: %s\n", buf);

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
