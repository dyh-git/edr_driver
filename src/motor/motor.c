/*
* motor.c - XXXX
*
* Copyright (C) 2020 XXX Technology Corp.
*
* Author: dyh
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/mod_devicetable.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <motor/motor.h>
#include <motor/motor_usr.h>

extern struct motor_dev g_motor_init __attribute__((weak));
struct motor_dev g_motor_init;

static struct motor_dev *g_motor_dev = NULL;

int motor_open(struct inode *node, struct file *filp)
{
	int ret = 0;
	struct motor_dev *motor_dev = container_of(filp->private_data, struct motor_dev, misc_dev);

	if (!motor_dev->motor_open) {
		printk_err("func motor_open is null.\n");
		return -1;
	}

	ret = motor_dev->motor_open(&motor_dev->chip_conf);
	if (ret < 0) {
		printk_err("open motor error.\n");
		
		return -1;
	}

    return 0;
}

int motor_release(struct inode *node, struct file *filp)
{
	int ret = 0;
	struct motor_dev *motor_dev = container_of(filp->private_data, struct motor_dev, misc_dev);

	if (!motor_dev->motor_close) {
		printk_err("func motor_close is null.\n");
		return -1;
	}

	ret = motor_dev->motor_close(&motor_dev->chip_conf);
	if (ret < 0) {
		printk_err("close motor error.\n");
		
		return -1;
	}

    return 0;
}

long motor_ioctl(struct file *filp, uint32_t cmd, unsigned long arg)
{
	long ret = 0;
	struct motor_dev *motor_dev = container_of(filp->private_data, struct motor_dev, misc_dev);

	if (!motor_dev->motor_ioctl) {
		printk_err("motor_ioctl is null.\n");

		return -1;
	}
	ret = motor_dev->motor_ioctl(motor_dev, cmd, arg);
	if (ret < 0) {
		printk_err("ioctl motor error.\n");

		return -1;
	}

	return ret;

}

struct file_operations motor_fops = {
    .owner          = THIS_MODULE,
    .open           = motor_open,
    .release        = motor_release,
    .unlocked_ioctl = motor_ioctl,
};

static int motor_probe(struct spi_device *spi)
{
	int ret = 0;
 
	printk("spi driver and device was matched!\n");
 
	g_motor_dev = kzalloc(sizeof(struct motor_dev), GFP_KERNEL);
	if (!g_motor_dev) {
        printk("kmalloc stepmotor dev fail.\n");
        return -1;
    }
 
	memcpy(g_motor_dev, &g_motor_init, sizeof(struct motor_dev));
	
	g_motor_dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
    g_motor_dev->misc_dev.name 	= MOTOR_DEV_NAME;
    g_motor_dev->misc_dev.fops  = &motor_fops;
	
    ret = misc_register(&g_motor_dev->misc_dev);
    if(unlikely(ret < 0)) {
        printk("register misc fail\n");
    }
	mutex_init(&g_motor_dev->buf_lock);
	
	spi->mode = SPI_MODE_0;
	spi_setup(spi);
 
	hrtimer_init(&g_motor_dev->chip_conf.tmi8150_chip.ms_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    g_motor_dev->chip_conf.tmi8150_chip.ms_timer.function = tmi8150_ms_timer_func;

 	switch (g_motor_dev->drv_chip) {
		case CHIP_TMI8150:
			g_motor_dev->motor_open  = tmi8150_open;
			g_motor_dev->motor_close = tmi8150_close;
			g_motor_dev->motor_read  = tmi8150_read;
			g_motor_dev->motor_ioctl = tmi8150_ioctl;
			g_motor_dev->chip_conf.tmi8150_chip.spi = spi;
			break;
		default:
			break;
	}

	return ret;
}
 
static int motor_remove(struct spi_device *spi)
{ 
    misc_deregister(&g_motor_dev->misc_dev);
 
	kfree(g_motor_dev);
	g_motor_dev = NULL;
	
    return 0;
}
 
static const struct spi_device_id motor_id_table[] = 
{
	{
	"ingenic,spi", 0},
	{
	},
};
 
static const struct of_device_id motor_of_match[] = 
{   
	{
	.compatible = "ingenic,spi"},
	{
	},
};
 
static struct spi_driver motor_driver = 
{
	.probe = motor_probe,
	.remove = motor_remove,
	.driver = {
		.name = "spi",
		.owner = THIS_MODULE,
		.of_match_table = motor_of_match,
	},
	.id_table = motor_id_table,
};

int pg_motor_init(void)
{
	spi_register_driver(&motor_driver);
	
    printk_log("%s module is installed\n", __func__);
 
    return 0;
}
 
void pg_motor_exit(void)
{	
	spi_unregister_driver(&motor_driver);
	
    printk_log("%s module is removed.\n", __func__);
	
    return;
}
