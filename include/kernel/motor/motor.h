#ifndef __MOTOR_H__
#define __MOTOR_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>

#include <config/pg_gpio.h>

#include "tmi8150b.h"

#define MOTOR_DEV_NAME		"pg_motor"

typedef enum motor_drv_chip {
	CHIP_TMI8150 = 1,
}motor_drv_chip_e;

typedef union motor_chip {
	tmi8150_attri_s tmi8150_chip;
}motor_chip_u;

typedef struct motor_dev {
	struct miscdevice 	misc_dev;
	struct mutex		buf_lock;
	
	motor_drv_chip_e	drv_chip;
	motor_chip_u chip_conf;
	
	int (*motor_open)(void *chip);
	int (*motor_close)(void *chip);
	int (*motor_read)(void *chip, char *buf, size_t count);
	long (*motor_ioctl)(void *chip, uint32_t cmd, unsigned long arg);
}motor_dev_s;

#endif


