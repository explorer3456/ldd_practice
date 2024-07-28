#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/linux/fs.h>

#define DEV_MEM_SIZE	8

// including EOF (DEV MEM SIZE + 1)
static char pcdev_buffer[DEV_MEM_SIZE];

dev_t device_number;

struct cdev pcd_cdev;
struct class * pcd_class;
struct device * pcd_dev;

#undef pr_fmt
#define pr_fmt(fmt) "[PCD_DEV][%s] : " fmt, __func__

loff_t pcd_llseek (struct file *filep, loff_t offset, int whence)
{
	int ret;

	ret = 0;

	pr_info("current file position: %lld\n", filep->f_pos);
	
	switch(whence) {
		case SEEK_SET:
			if ( (offset > DEV_MEM_SIZE) || (offset < 0) ){
				pr_info("the address is beyond the address\n");
				ret = -EINVAL;
			} else {
				filep->f_pos = offset;
			}

			break;
		case SEEK_CUR:
			if ( ((filep->f_pos + offset) > DEV_MEM_SIZE) || ( (filep->f_pos + offset) < 0) ) {
				pr_info("the address is beyond the address\n");
				ret = -EINVAL;
			} else {
				filep->f_pos = filep->f_pos + offset;
			}
			break;
		case SEEK_END:
			if (offset > 0 ) {
				pr_info("the address is beyond the address\n");
				ret = -EINVAL;
			} else {
				filep->f_pos = DEV_MEM_SIZE + offset;
			}
			break;
		default:
			pr_info("unkown whence\n");
			ret = -EINVAL;
			break;
	}
	pr_info("whence: %d, offset: %lld\n", whence, offset);
	pr_info("updated file position to : %lld\n", filep->f_pos);

	if (ret < 0 )
		return ret;
	else
		return filep->f_pos;
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

	// if f_pos is end of file, dont access to that location.
	if ( ((*f_pos) < DEV_MEM_SIZE) ) {
		read_bytes = copy_to_user(buf, (void *)pcdev_buffer + (*f_pos), count_adj);

		// there is some error. Address error.
		if (read_bytes != 0) {
			if (read_bytes < 0 ) {
				return -EFAULT;
			} else if (read_bytes > 0 ) {
				pr_info("ERROR. copy  to use failed: %d\n", read_bytes);
			}

		}
	}

	// update file position.
	*f_pos = *f_pos + count_adj;
	pr_info("updated file position to : %lld\n", *f_pos);

	// copy to user.

	return count_adj;;
};

ssize_t pcd_write (struct file *filep, const char __user *buf, size_t count, loff_t * f_pos)
{
	int count_adj;
	int write_bytes;

	pr_info("user request: %zu\n", count);
	pr_info("current file position: %lld\n", *f_pos);

	count_adj = count;
	write_bytes = 0;

	if ( (*f_pos) + count_adj > DEV_MEM_SIZE) {
		count_adj = DEV_MEM_SIZE - (*f_pos);
		pr_info("adjust count to : %d\n", count_adj);
	}

	// end of file
	if (count_adj == 0) {
		pr_err("No space left in device\n");
		return -ENOMEM;
	}


	// if f_pos is end of file, dont access to that location.
	if ( ((*f_pos) < DEV_MEM_SIZE) ) {
		write_bytes = copy_from_user( (void *)pcdev_buffer + (*f_pos), buf, count_adj);

		// there is some error. Address error.
		// or there are some bytes that cannot be coppied.
		if ( (write_bytes != 0) ) {

			if (write_bytes < 0 ) {
				return -EFAULT;
			} else if (write_bytes > 0 ) {
				pr_info("ERROR. copy from user failed: %d\n", write_bytes);
			}
		}
	}

	// update file position.
	*f_pos = *f_pos + count_adj;

	pr_info("updated file position to : %lld\n", *f_pos);

	return count_adj;
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

	ret = 0;

	/* device number */
	/* we need device number to register our device file to VFS. That is why we allocate device number */ 
	ret = alloc_chrdev_region( &device_number, 0, 1, "pcd_device_num");
	if (ret < 0) {
		pr_err("alloc chrdev region failed: %d\n", ret);
		goto err_chrdev_fail;
	}

	pr_info("major: %d, minor: %d \n", MAJOR(device_number), MINOR(device_number));

	/* cdev init. */
	/* we want to register our device to VFS. That is why we initialize cdev. */
	cdev_init( &pcd_cdev, &pcd_fops);

	pcd_cdev.owner = THIS_MODULE;

	/* we want to register char device to VFS. */ 
	ret = cdev_add( &pcd_cdev, device_number, 1);
	if (ret < 0) {
		pr_err("cdev add failed: %d\n", ret);
		goto err_cdev_add_fail;
	}

	/* since we have registered our char dev information and device number to VFS using cdev_add, 
	 we want to create device file. 
	 we can expose our device file information, which is device number to sysfs directory so that 
	 udev program will create device file.*/

	pcd_class = class_create(pcd_cdev.owner, "pcd_class");
	if (IS_ERR(pcd_class)) {
		ret = PTR_ERR(pcd_class);
		pr_err("class create failed: %ld\n", ret);
		goto err_class_fail;
	}
	pcd_dev = device_create( pcd_class, NULL, device_number, NULL, "pcd_dev");
	if (IS_ERR(pcd_dev)) {
		ret = PTR_ERR(pcd_dev);
		pr_err("device create failed: %ld\n", ret);
		goto err_device_fail;
	}

err_device_fail:
	class_destroy(pcd_class);
err_class_fail:
	cdev_del( &pcd_cdev );
err_cdev_add_fail:
	unregister_chrdev_region( device_number, 1);
err_chrdev_fail:
	pr_err("module init failed: %d\n", ret);
	return ret;
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
