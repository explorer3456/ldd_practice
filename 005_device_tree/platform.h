#ifndef __H_PLATFORM_H__
#define __H_PLATFORM_H__

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/cdev.h>

#define DEV_MEM_SIZE	8

#define DEV_MEM_SIZE0	8
#define DEV_MEM_SIZE1	16	
#define DEV_MEM_SIZE2	24
#define DEV_MEM_SIZE3	32

#define NUM_OF_DEVICES	4

#define PERM_READ_ONLY	1
#define PERM_WRITE_ONLY 2
#define PERM_READ_WRITE	3

struct pcdev_platform_data {
	unsigned int size;
	const char * serial_number;
	int perm;
};

#endif
