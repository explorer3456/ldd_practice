#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

dev_t device_number;

struct cdev pcd_cdev;
struct class * pcd_class;
struct device * pcd_dev;

#undef pr_fmt
#define pr_fmt(fmt) "[PCD_DEV][%s] : " fmt, __func__

loff_t pcd_llseek (struct file *filep, loff_t offset, int whence)
{
	pr_info("\n");
	return 0;
};

ssize_t pcd_read (struct file *filep, char __user *buf, size_t count, loff_t * offset)
{
	pr_info("\n");
	return 0;
};

ssize_t pcd_write (struct file *filep, const char __user *buf, size_t count, loff_t * offset)
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

static int __init pcd_module_init(void)
{
	int ret;

	/* device number */
	/* we need device number to register our device file to VFS. That is why we allocate device number */ 
	ret = alloc_chrdev_region( &device_number, 0, 1, "pcd_device_num");

	pr_info("major: %d, minor: %d \n", MAJOR(device_number), MINOR(device_number));

	/* cdev init. */
	/* we want to register our device to VFS. That is why we initialize cdev. */
	cdev_init( &pcd_cdev, &pcd_fops);

	pcd_cdev.owner = THIS_MODULE;

	/* we want to register char device to VFS. */ 
	cdev_add( &pcd_cdev, device_number, 1);

	/* since we have registered our char dev information and device number to VFS using cdev_add, 
	 we want to create device file. 
	 we can expose our device file information, which is device number to sysfs directory so that 
	 udev program will create device file.*/

	pcd_class = class_create(pcd_cdev.owner, "pcd_class");
	pcd_dev = device_create( pcd_class, NULL, device_number, NULL, "pcd_dev");

	return 0;
}

static void __exit pcd_module_exit(void)
{
	device_destroy( pcd_class, device_number);
	class_destroy( pcd_class ); 
	cdev_del( &pcd_cdev );
	unregister_chrdev_region( device_number, 1);

	pr_info("moduel cleanup\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
