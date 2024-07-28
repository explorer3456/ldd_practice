#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno-base.h>

#define DEV_MEM_SIZE	512

// including EOF (DEV MEM SIZE + 1)
static char pcdev_buffer[DEV_MEM_SIZE + 1];

dev_t device_number;

struct cdev pcd_cdev;
struct class * pcd_class;
struct device * pcd_dev;

#undef pr_fmt
#define pr_fmt(fmt) "[PCD_DEV][%s] : " fmt, __func__

loff_t pcd_llseek (struct file *filep, loff_t f_pos, int whence)
{
	pr_info("\n");
	return 0;
};

ssize_t pcd_read (struct file *filep, char __user *buf, size_t count, loff_t * f_pos)
{
	int count_adj;
	int read_bytes;

	read_bytes = 0;
	count_adj = count;
	// check count value valid.

	pr_info("user request: %zu\n", count);
	pr_info("current file position: %lld\n", *f_pos);

	if ( ((*f_pos) + count_adj) > DEV_MEM_SIZE) {
		// adjust count
		count_adj = DEV_MEM_SIZE - (*f_pos);
		pr_info("adjust count to : %d\n", count_adj);
	}

	read_bytes = copy_to_user(buf, (void *)pcdev_buffer + (*f_pos), count_adj);
	// there is some error. Address error.
	if (read_bytes < 0 ) {
		return -EFAULT;
	}

	// update file position.
	*f_pos = *f_pos + count_adj;
	pr_info("read bytes: %d\n", read_bytes);
	pr_info("updated file position to : %lld\n", *f_pos);

	// copy to user.

	return read_bytes;
};

ssize_t pcd_write (struct file *filep, const char __user *buf, size_t count, loff_t * f_pos)
{
	int count_adj;
	int write_bytes;

	pr_info("user request: %zu\n", count);
	pr_info("current file position: %lld\n", *f_pos);

	count_adj = count;


	if ( (*f_pos) + count_adj > DEV_MEM_SIZE) {
		count_adj = DEV_MEM_SIZE - (*f_pos);
	}

	// end of file
	if (count_adj == 0)
		return -ENOMEM;

	write_bytes = copy_from_user( (void *)pcdev_buffer + (*f_pos), buf, count_adj);

	// there is some error. Address error.
	if (write_bytes < 0) {
		return -EFAULT;
	}

	// update file position.
	*f_pos = *f_pos + count_adj;

	pr_info("write bytes: %d\n", write_bytes);
	pr_info("updated file position to : %lld\n", *f_pos);

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
