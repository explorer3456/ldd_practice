#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include "platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "[PCD_DEV][%s] : " fmt, __func__

struct pcdev_platform_data pcdev_priv[3] = {
	[0] = {
		.size = DEV_MEM_SIZE0,
		.serial_number = "pcd_priv_serial-0",
		.perm = PERM_READ_ONLY,
	},
	[1] = {
		.size = DEV_MEM_SIZE1,
		.serial_number = "pcd_priv_serial-1",
		.perm = PERM_READ_WRITE,
	},
	[2] = {
		.size = DEV_MEM_SIZE2,
		.serial_number = "pcd_priv_serial-def",
		.perm = PERM_READ_WRITE,
	},
};

static void pcd_plat_dev_release(struct device *dev)
{
	pr_info("device release\n");
}

struct platform_device pcd_plat_dev0 = {
	.name = "pcd_plat_dev-v1.0",
	.id = 1,
	.dev = {
		.release = pcd_plat_dev_release,
		.platform_data = (struct pcdev_platform_data *)&pcdev_priv[0],
	},
};

struct platform_device pcd_plat_dev1 = {
	.name = "pcd_plat_dev-v2.0",
	.id = 2,
	.dev = {
		.release = pcd_plat_dev_release,
		.platform_data = (struct pcdev_platform_data *)&pcdev_priv[1],
	},
};

struct platform_device pcd_plat_dev_def = {
	.name = "pcd_plat_dev",
	.id = 0,
	.dev = {
		.release = pcd_plat_dev_release,
		.platform_data = (struct pcdev_platform_data *)&pcdev_priv[2],
	},
};

struct platform_device *pcd_plat_dev[] = {
	&pcd_plat_dev0,
	&pcd_plat_dev1,
	&pcd_plat_dev_def,
};

static int __init plat_device_init(void)
{
	int ret;

	pr_info("platform device module init start\n");
	// ret = platform_device_register( &pcd_plat_dev0 );
	// ret = platform_device_register( &pcd_plat_dev1 );
	ret = platform_add_devices( pcd_plat_dev, ARRAY_SIZE( pcd_plat_dev ));

	if (ret < 0 ) {
		pr_err("platform device register failed: %d\n", ret);
	} else {
		pr_info("platform device module init\n");
	}

	return ret;
}

static void __exit plat_device_cleanup(void)
{
	platform_device_unregister(&pcd_plat_dev0);
	platform_device_unregister(&pcd_plat_dev1);
	platform_device_unregister(&pcd_plat_dev_def);
}

module_init(plat_device_init);
module_exit(plat_device_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
