#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/linux/fs.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/spinlock.h>

#define DEV_MEM_SIZE	8

#define DEV_MEM_SIZE0	8
#define DEV_MEM_SIZE1	16	
#define DEV_MEM_SIZE2	24
#define DEV_MEM_SIZE3	32

#define NUM_OF_DEVICES	4

#define PERM_READ_ONLY	1
#define PERM_WRITE_ONLY 2
#define PERM_READ_WRITE	3


static char pcdev0_buffer[DEV_MEM_SIZE0];
static char pcdev1_buffer[DEV_MEM_SIZE1];
static char pcdev2_buffer[DEV_MEM_SIZE2];
static char pcdev3_buffer[DEV_MEM_SIZE3];

// since there are multiple devices, we should keep device information 
// to array.
struct pcdev_private_data {
	char * buffer;
	unsigned int size;
	const char * serial_number;
	int perm;
	struct cdev pcd_cdev;
	spinlock_t pcd_lock;
};

struct pcdrv_private_data {
	int total_devices;
	struct pcdev_private_data pcdev_priv[NUM_OF_DEVICES];

	dev_t device_number;
	struct class * pcd_class;
};

struct pcdrv_private_data pcdrv_priv = {
	.total_devices = NUM_OF_DEVICES,
	.pcdev_priv[0] = {
		.buffer = pcdev0_buffer,
		.size = DEV_MEM_SIZE0,
		.serial_number = "pcdev-serial0",
		.perm = PERM_READ_ONLY,
	},
	.pcdev_priv[1] = {
		.buffer = pcdev1_buffer,
		.size = DEV_MEM_SIZE1,
		.serial_number = "pcdev-serial1",
		.perm = (PERM_READ_WRITE),
	},
	.pcdev_priv[2] = {
		.buffer = pcdev2_buffer,
		.size = DEV_MEM_SIZE2,
		.serial_number = "pcdev-serial2",
		.perm = PERM_WRITE_ONLY,
	},
	.pcdev_priv[3] = {
		.buffer = pcdev3_buffer,
		.size = DEV_MEM_SIZE3,
		.serial_number = "pcdev-serial3",
		.perm = PERM_READ_ONLY,
	},
};

// including EOF (DEV MEM SIZE + 1)
// static char pcdev_buffer[DEV_MEM_SIZE];

dev_t device_number;

struct class * pcd_class;

struct device * pcd_dev;

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
#define pr_fmt(fmt) "[PCD_DEV][%s] : " fmt, __func__

loff_t pcd_llseek (struct file *filep, loff_t offset, int whence)
{
	int ret;
	struct pcdev_private_data *pcd_priv;

	pcd_priv = (struct pcdev_private_data *)filep->private_data;

	ret = 0;

	pr_info("current file position: %lld\n", filep->f_pos);
	
	switch(whence) {
		case SEEK_SET:
			if ( (offset > pcd_priv->size) || (offset < 0) ){
				pr_info("the address is beyond the address\n");
				ret = -EINVAL;
			} else {
				filep->f_pos = offset;
			}

			break;
		case SEEK_CUR:
			if ( ((filep->f_pos + offset) > pcd_priv->size) || ( (filep->f_pos + offset) < 0) ) {
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
				filep->f_pos = pcd_priv->size + offset;
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
	struct pcdev_private_data *pcd_priv;

	pcd_priv = (struct pcdev_private_data *)filep->private_data;

	read_bytes = 0;
	count_adj = count;
	// check count value valid.

	pr_info("serial number: %s\n", pcd_priv->serial_number);
	pr_info("user request: %zu\n", count);
	pr_info("current file position: %lld\n", *f_pos);

	if ( ((*f_pos) + count_adj) > pcd_priv->size) {
		// adjust count
		count_adj = (pcd_priv->size) - (*f_pos);
		pr_info("adjust count to : %d\n", count_adj);
	}

	// if f_pos is end of file, dont access to that location.
	if ( ((*f_pos) < (pcd_priv->size)) ) {
		read_bytes = copy_to_user(buf, (void *)pcd_priv->buffer + (*f_pos), count_adj);

		// there is some error. Address error.
		if (read_bytes != 0) {
			if (read_bytes < 0 ) {
				return -EFAULT;
			} else if (read_bytes > 0 ) {
				pr_err("ERROR. copy  to use failed: %d\n", read_bytes);
				return -EFAULT;
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
	unsigned long flag;
	struct pcdev_private_data *pcd_priv;

	pcd_priv = (struct pcdev_private_data *)filep->private_data;

	spin_lock_irqsave( &pcd_priv->pcd_lock, flag);

	pr_info("user request: %zu\n", count);
	pr_info("current file position: %lld\n", *f_pos);

	count_adj = count;
	write_bytes = 0;

	if ( (*f_pos) + count_adj > (pcd_priv->size)) {
		count_adj = (pcd_priv->size) - (*f_pos);
		pr_info("adjust count to : %d\n", count_adj);
	}

	// end of file
	if (count_adj == 0) {
		pr_err("No space left in device\n");
		return -ENOMEM;
	}


	// if f_pos is end of file, dont access to that location.
	if ( ((*f_pos) < (pcd_priv->size)) ) {

		write_bytes = copy_from_user( (void *)pcd_priv->buffer + (*f_pos), buf, count_adj);

		// there is some error. Address error.
		// or there are some bytes that cannot be coppied.
		if ( (write_bytes != 0) ) {

			if (write_bytes < 0 ) {
				return -EFAULT;
			} else if (write_bytes > 0 ) {
				pr_err("ERROR. copy from user failed: %d\n", write_bytes);
				return -EFAULT;
			}
		}
	}

	// update file position.
	*f_pos = *f_pos + count_adj;

	pr_info("updated file position to : %lld\n", *f_pos);

	spin_unlock_irqrestore( &pcd_priv->pcd_lock, flag);

	return count_adj;
};

int pcd_open (struct inode *inodep, struct file *filep)
{
	dev_t device_number;
	int dev_minor;
	struct pcdev_private_data * dev_priv;
	/* you can get which devices from multi devices using inode. */

	device_number = inodep->i_rdev;

	dev_minor = MINOR( device_number );
	
	pr_info("minor number: %d\n", dev_minor);

	/* get devices private data structure */

	dev_priv = container_of( inodep->i_cdev, struct pcdev_private_data, pcd_cdev);

	filep->private_data = dev_priv;

	/* check permission */
	pr_info(" permission: %d\n", dev_priv->perm);

	if (!check_permission(filep->f_mode, dev_priv->perm)) {
		return -EPERM;
	}

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
	int i;
	int cdev_fail_idx;
	int dev_create_fail_idx;

	ret = 0;
	cdev_fail_idx = 0;
	dev_create_fail_idx = 0;

	/* device number */
	/* we need device number to register our device file to VFS. That is why we allocate device number */ 
	ret = alloc_chrdev_region( &pcdrv_priv.device_number, 0, NUM_OF_DEVICES, "pcd_device_num");
	if (ret < 0) {
		pr_err("alloc chrdev region failed: %d\n", ret);
		goto err_chrdev_fail;
	}


	for (i=0; i<NUM_OF_DEVICES; i++) {

		pr_info("devnumber: %08x, major: %d, minor: %d \n", pcdrv_priv.device_number + i, MAJOR(pcdrv_priv.device_number + i), MINOR(pcdrv_priv.device_number+ i));

		/* cdev init. */
		/* we want to register our device to VFS. That is why we initialize cdev. */
		cdev_init( &(pcdrv_priv.pcdev_priv[i].pcd_cdev), &pcd_fops);
		pcdrv_priv.pcdev_priv[i].pcd_cdev.owner = THIS_MODULE;

		spin_lock_init(&pcdrv_priv.pcdev_priv[i].pcd_lock);

		/* we want to register char device to VFS. */ 
		ret = cdev_add( &pcdrv_priv.pcdev_priv[i].pcd_cdev, pcdrv_priv.device_number + i, 1);
		if (ret < 0) {
			pr_err("cdev add failed: %d\n", ret);
			cdev_fail_idx = i;
			goto err_cdev_add_fail;
		}
	}

	/* since we have registered our char dev information and device number to VFS using cdev_add, 
	 we want to create device file. 
	 we can expose our device file information, which is device number to sysfs directory so that 
	 udev program will create device file.*/

	/* we need only one class directory. so we add pcdev0 as arguement. */
	pcdrv_priv.pcd_class = class_create( THIS_MODULE, "pcd_class");
	if (IS_ERR(pcdrv_priv.pcd_class)) {
		ret = PTR_ERR(pcdrv_priv.pcd_class);
		pr_err("class create failed: %d\n", ret);
		goto err_class_fail;
	}

	for (i=0; i<NUM_OF_DEVICES; i++){
		pcd_dev = device_create( pcdrv_priv.pcd_class, NULL, pcdrv_priv.device_number + i, NULL,\
				"pcd-dev-create-%d", i);
		if (IS_ERR(pcd_dev)) {
			ret = PTR_ERR(pcd_dev);
			dev_create_fail_idx = i;
			pr_err("device create failed: %d\n", ret);
			goto err_device_fail;
		}
	}

	return ret;

err_device_fail:
	pr_err("err device fail\n");
	for(i=0; i<NUM_OF_DEVICES; i++) {
		device_destroy( pcdrv_priv.pcd_class, pcdrv_priv.device_number + i);
	}
	class_destroy( pcdrv_priv.pcd_class);
err_class_fail:
	pr_err("err class fail\n");
err_cdev_add_fail:
	pr_err("err cdev add fail\n");
	for(i=0; i<NUM_OF_DEVICES; i++) {
		cdev_del(&pcdrv_priv.pcdev_priv[i].pcd_cdev);
	}
	unregister_chrdev_region( pcdrv_priv.device_number, NUM_OF_DEVICES);
err_chrdev_fail:
	pr_err("module init failed: %d\n", ret);

	return ret;
}

static void __exit pcd_module_exit(void)
{
	int i;
	
	for(i=0; i<NUM_OF_DEVICES; i++) {
		device_destroy( pcdrv_priv.pcd_class, pcdrv_priv.device_number + i);
	}
	class_destroy( pcdrv_priv.pcd_class ); 

	for(i=0; i<NUM_OF_DEVICES; i++) {
		cdev_del( &pcdrv_priv.pcdev_priv[i].pcd_cdev);
	}
	unregister_chrdev_region( pcdrv_priv.device_number, NUM_OF_DEVICES);

	pr_info("moduel cleanup\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
