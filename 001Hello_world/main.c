#include <linux/module.h>

static int __init hello_world_init(void)
{
	pr_info("Hello world\n");
	return 0;
}

static void __exit hello_world_exit(void)
{
	pr_info("Good bye\n");
}

module_init(hello_world_init);
module_exit(hello_world_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Simple hello world kernel module");
MODULE_INFO(board, "Beaglebone black REV A5");
