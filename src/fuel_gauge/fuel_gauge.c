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
#include <linux/i2c.h>

#include <fuel_gauge/fuel_gauge.h>

extern struct fuel_gauge_dev g_fuel_gauge_init __attribute__((weak));
struct fuel_gauge_dev g_fuel_gauge_init;
static struct fuel_gauge_dev *g_fuel_gauge_dev = NULL;

static int fuel_gauge_open(struct inode *node, struct file *filp)
{
	int ret = 0;
	struct fuel_gauge_dev *fuel_gauge_dev = container_of(filp->private_data, struct fuel_gauge_dev, misc_dev);
	
	if (!fuel_gauge_dev->fuel_gauge_open) {
		printk_err("func fuel_gauge_open is null.\n");
		return -1;
	}

	ret = fuel_gauge_dev->fuel_gauge_open(&fuel_gauge_dev->chip_conf);
	if (ret < 0) {
		printk_err("open fuel_gauge error.\n");
		
		return -1;
	}
	
    return 0;
}

static int fuel_gauge_release(struct inode *node, struct file *filp)
{
	int ret = 0;
	struct fuel_gauge_dev *fuel_gauge_dev = container_of(filp->private_data, struct fuel_gauge_dev, misc_dev);

	if (!fuel_gauge_dev->fuel_gauge_close) {
		printk_err("func fuel_gauge_close is null.\n");
		return -1;
	}

	ret = fuel_gauge_dev->fuel_gauge_close(&fuel_gauge_dev->chip_conf);
	if (ret < 0) {
		printk_err("close fuel_gauge error.\n");
		
		return -1;
	}
	
    return 0;
}

static ssize_t fuel_gauge_read(struct file *filp, char __user *buf, size_t count, loff_t * ppos)
{
	int ret = 0;
	uint8_t data[32] = {0x00};
	struct fuel_gauge_dev *fuel_gauge_dev = container_of(filp->private_data, struct fuel_gauge_dev, misc_dev);

	mutex_lock(&fuel_gauge_dev->buf_lock);	
	printk_log("fuel_gauge_read.....\n");
	if (copy_from_user(data, (void *)buf, 1)) {
		printk("error: copy from user\n");
		
		ret = -1;
		goto err;
    }

	memset(data, 0x00, 32);
	if (count > 32)
		count = 32;

	if (!fuel_gauge_dev->fuel_gauge_read) {
		printk_err("func fuel_gauge_read is null.\n");

		ret = -1;
		goto err;
	}	
	ret = fuel_gauge_dev->fuel_gauge_read(&fuel_gauge_dev->chip_conf, data, count);
	if (ret < 0) {
		printk_err("read fuel_gauge data error.\n");
		
		ret = -1;
		goto err;
	}
	
	if (copy_to_user(buf, (void *)data, count)) {
		printk("error: copy to user\n");
		
		ret = -1;
		goto err;
    }	

err:
	mutex_unlock(&fuel_gauge_dev->buf_lock);
	
	return ret;
}

struct file_operations fuel_gauge_fops = {
    .owner          = THIS_MODULE,
    .open           = fuel_gauge_open,
    .release        = fuel_gauge_release,
	.read			= fuel_gauge_read,
};

static int fuel_gauge_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	
	printk_log("%s i2c driver and device was matched!\n", __func__);
	
	g_fuel_gauge_dev = kzalloc(sizeof(struct fuel_gauge_dev), GFP_KERNEL);
	if (!g_fuel_gauge_dev) {
		printk_err("kmalloc fuel_gauge dev error.\n");
		
		return -1;
	}
	
	memcpy(g_fuel_gauge_dev, &g_fuel_gauge_init, sizeof(struct fuel_gauge_dev));
	
	g_fuel_gauge_dev->misc_dev.minor 	= MISC_DYNAMIC_MINOR;
	g_fuel_gauge_dev->misc_dev.name 	= FUEL_GAUGE_DEV_NAME;
	g_fuel_gauge_dev->misc_dev.fops  	= &fuel_gauge_fops;
	
	ret = misc_register(&g_fuel_gauge_dev->misc_dev);
	if(unlikely(ret < 0)) {
		printk("register misc fail\n");
	}
	mutex_init(&g_fuel_gauge_dev->buf_lock);
	
	switch (g_fuel_gauge_dev->drv_chip) {
		case CHIP_CW221X:
			g_fuel_gauge_dev->fuel_gauge_open  = cw221x_open;
			g_fuel_gauge_dev->fuel_gauge_close = cw221x_close;
			g_fuel_gauge_dev->fuel_gauge_read  = cw221x_read;
			g_fuel_gauge_dev->chip_conf.cw221x_chip.client = client; 
			break;
		default:
			break;
	}
	
	return ret;
}

static int fuel_gauge_remove(struct i2c_client *client)
{
	misc_deregister(&g_fuel_gauge_dev->misc_dev);

	kfree(g_fuel_gauge_dev);
	g_fuel_gauge_dev = NULL;

	return 0;
}

static const struct i2c_device_id fuel_gauge_id[] = {
	{"ingenic,t40-i2c", 0},
	{}
};
 
static const struct of_device_id fuel_gauge_of_device[] = {
	{.compatible = "ingenic,t40-i2c"},
	{ /* sentinel */ }
};
 
static struct i2c_driver fuel_gauge_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "t40-i2c",
		.of_match_table = fuel_gauge_of_device,
	},
	.id_table = fuel_gauge_id,
	.probe = fuel_gauge_probe,
	.remove = fuel_gauge_remove,
};

int pg_fuel_gauge_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&fuel_gauge_driver);

	printk_log("%s module is installed\n", __func__);

	return ret;
}
 
void pg_fuel_gauge_exit(void)
{
	i2c_del_driver(&fuel_gauge_driver);

	printk_log("%s module is removed.\n", __func__);
}
