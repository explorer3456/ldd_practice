#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#undef pr_fmt
#define pr_fmt(fmt) "[PCD_DEV][%s] : " fmt, __func__

static void pcd_plat_dev_release(struct device *dev)
{
	pr_info("device release\n");
}

struct platform_device pcd_plat_dev = {
	.name = "pcd_plat_dev-0",
	.dev = {
		.release = pcd_plat_dev_release,
	},
};

static int __init plat_device_init(void)
{
	int ret;

	pr_info("platform device module init start\n");
	ret = platform_device_register( &pcd_plat_dev );

	if (ret < 0 ) {
		pr_err("platform device register failed: %d\n", ret);
	} else {
		pr_info("platform device module init\n");
	}

	return ret;
}

static void __exit plat_device_cleanup(void)
{
	platform_device_unregister(&pcd_plat_dev);
}

module_init(plat_device_init);
module_exit(plat_device_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
