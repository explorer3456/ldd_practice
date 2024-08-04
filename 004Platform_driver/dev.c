#include <linux/module.h>
#include <linux/platform_device.h>

struct platform_device pcd_plat_dev = {
	.name = "pcd_plat_dev-0",
};

static int __init plat_device_init(void)
{
	int ret;

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
