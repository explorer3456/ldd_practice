#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

dev_t device_number;

struct cdev pcd_cdev;
struct file_operations pcd_fops = {
	.owner = THIS_MODULE,
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


	return 0;
}

static void __exit pcd_module_exit(void)
{
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
