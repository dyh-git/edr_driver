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

#include <acc_gyro/acc_gyro.h>

extern struct acc_gyro_dev g_acc_gyro_init __attribute__((weak));
struct acc_gyro_dev g_acc_gyro_init;
static struct acc_gyro_dev *g_acc_gyro_dev = NULL;

static int acc_gyro_open(struct inode *node, struct file *filp)
{
	int ret = 0;
	struct acc_gyro_dev *acc_gyro_dev = container_of(filp->private_data, struct acc_gyro_dev, misc_dev);
	
	if (!acc_gyro_dev->acc_gyro_open) {
		printk_err("func acc_gyro_open is null.\n");
		return -1;
	}
	ret = acc_gyro_dev->acc_gyro_open(&acc_gyro_dev->chip_conf);
	if (ret < 0) {
		printk_err("open acc_gyro error.\n");
		
		return -1;
	}
	
    return 0;
}

static int acc_gyro_release(struct inode *node, struct file *filp)
{
	int ret = 0;
	struct acc_gyro_dev *acc_gyro_dev = container_of(filp->private_data, struct acc_gyro_dev, misc_dev);

	if (!acc_gyro_dev->acc_gyro_close) {
		printk_err("func acc_gyro_close is null.\n");
		return -1;
	}
	ret = acc_gyro_dev->acc_gyro_close(&acc_gyro_dev->chip_conf);
	if (ret < 0) {
		printk_err("close acc_gyro error.\n");
		
		return -1;
	}
	
    return 0;
}

static ssize_t acc_gyro_read(struct file *filp, char __user *buf, size_t count, loff_t * ppos)
{
	int ret = 0;
	uint8_t data[32] = {0x00};
	struct acc_gyro_dev *acc_gyro_dev = container_of(filp->private_data, struct acc_gyro_dev, misc_dev);

	mutex_lock(&acc_gyro_dev->buf_lock);	
	printk_log("acc_gyro_read.....\n");
	if (copy_from_user(data, (void *)buf, 1)) {
		printk("error: copy from user\n");
		
		ret = -1;
		goto err;
    }

	memset(data, 0x00, 32);
	if (count > 32)
		count = 32;

	if (!acc_gyro_dev->acc_gyro_read) {
		printk_err("func acc_gyro_read is null.\n");

		ret = -1;
		goto err;
	}	
	ret = acc_gyro_dev->acc_gyro_read(&acc_gyro_dev->chip_conf, data, count);
	if (ret < 0) {
		printk_err("read acc_gyro data error.\n");
		
		ret = -1;
		goto err;
	}
	
	if (copy_to_user(buf, (void *)data, count)) {
		printk("error: copy to user\n");
		
		ret = -1;
		goto err;
    }	

err:
	mutex_unlock(&acc_gyro_dev->buf_lock);
	
	return ret;
}

struct file_operations acc_gyro_fops = {
    .owner          = THIS_MODULE,
    .open           = acc_gyro_open,
    .release        = acc_gyro_release,
	.read			= acc_gyro_read,
};

static int acc_gyro_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	
	printk_log("%s i2c driver and device was matched!\n", __func__);
	
	g_acc_gyro_dev = kzalloc(sizeof(struct acc_gyro_dev), GFP_KERNEL);
	if (!g_acc_gyro_dev) {
		printk_err("kmalloc acc_gyro dev error.\n");
		
		return -1;
	}
	
	memcpy(g_acc_gyro_dev, &g_acc_gyro_init, sizeof(struct acc_gyro_dev));
	
	g_acc_gyro_dev->misc_dev.minor 	= MISC_DYNAMIC_MINOR;
	g_acc_gyro_dev->misc_dev.name 	= ACC_GYRO_DEV_NAME;
	g_acc_gyro_dev->misc_dev.fops  	= &acc_gyro_fops;
	
	ret = misc_register(&g_acc_gyro_dev->misc_dev);
	if(unlikely(ret < 0)) {
		printk("register misc fail\n");
	}
	mutex_init(&g_acc_gyro_dev->buf_lock);
	
	switch (g_acc_gyro_dev->drv_chip) {
		case CHIP_QMI8658:
			g_acc_gyro_dev->acc_gyro_open  = qmi8658_open;
			g_acc_gyro_dev->acc_gyro_close = qmi8658_close;
			g_acc_gyro_dev->acc_gyro_read  = qmi8658_read;
			g_acc_gyro_dev->chip_conf.qmi8658_chip.client = client; 
			break;
		default:
			break;
	}
	
	return ret;
}

static int acc_gyro_remove(struct i2c_client *client)
{
	misc_deregister(&g_acc_gyro_dev->misc_dev);

	kfree(g_acc_gyro_dev);
	g_acc_gyro_dev = NULL;

	return 0;
}

static const struct i2c_device_id acc_gyro_id[] = {
	{"ingenic,t40-i2c", 0},
	{}
};
 
static const struct of_device_id acc_gyro_of_device[] = {
	{.compatible = "ingenic,t40-i2c"},
	{ /* sentinel */ }
};
 
static struct i2c_driver acc_gyro_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "t40-i2c",
		.of_match_table = acc_gyro_of_device,
	},
	.id_table = acc_gyro_id,
	.probe = acc_gyro_probe,
	.remove = acc_gyro_remove,
};

int pg_acc_gyro_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&acc_gyro_driver);

	printk_log("%s module is installed\n", __func__);

	return ret;
}
 
void pg_acc_gyro_exit(void)
{
	i2c_del_driver(&acc_gyro_driver);

	printk_log("%s module is removed.\n", __func__);
}
