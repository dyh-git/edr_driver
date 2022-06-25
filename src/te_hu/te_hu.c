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

#include <te_hu/te_hu.h>
 
extern struct te_hu_dev g_te_hu_init __attribute__((weak));
struct te_hu_dev g_te_hu_init;
 
static struct te_hu_dev *g_te_hu_dev = NULL;
 
static int gen_te_hu_open(struct inode *node, struct file *filp)
{
	int ret = 0;
	struct te_hu_dev *te_hu_dev = container_of(filp->private_data, struct te_hu_dev, misc_dev);
	
	if (!te_hu_dev->te_hu_open) {
		printk_err("func te_hu_open is null.\n");
		return -1;
	}
	ret = te_hu_dev->te_hu_open(&te_hu_dev->chip_conf);
	if (ret < 0) {
		printk_err("open te_hu error.\n");
		
		return -1;
	}
	
    return 0;
}
 
static int gen_te_hu_release(struct inode *node, struct file *filp)
{
 	int ret = 0;
	struct te_hu_dev *te_hu_dev = container_of(filp->private_data, struct te_hu_dev, misc_dev);

	if (!te_hu_dev->te_hu_close) {
		printk_err("func te_hu_close is null.\n");
		return -1;
	}
	ret = te_hu_dev->te_hu_close(&te_hu_dev->chip_conf);
	if (ret < 0) {
		printk_err("close te_hu error.\n");
		
		return -1;
	}
	
    return 0;
}

static ssize_t gen_te_hu_read(struct file *filp, char __user *buf, size_t count, loff_t * ppos)
{
	int ret = 0;
	uint8_t data[8] = {0x00};
	struct te_hu_dev *te_hu_dev = container_of(filp->private_data, struct te_hu_dev, misc_dev);

	mutex_lock(&te_hu_dev->buf_lock);	
	printk_log("te_hu_read.....\n");
	
	memset(data, 0x00, 8);
	if (count > 8)
		count = 8;

	if (!te_hu_dev->te_hu_read) {
		printk_err("func te_hu_read is null.\n");

		ret = -1;
		goto err;
	}	
	ret = te_hu_dev->te_hu_read(&te_hu_dev->chip_conf, data, count);
	if (ret < 0) {
		printk_err("read te_hu data error.\n");
		
		ret = -1;
		goto err;
	}
	
	if (copy_to_user(buf, (void *)data, count)) {
		printk("error: copy from user\n");
		
		ret = -1;
		goto err;
    }	

err:
	mutex_unlock(&te_hu_dev->buf_lock);
	
	return ret;
}

struct file_operations gen_te_hu_fops = {
    .owner          = THIS_MODULE,
    .open           = gen_te_hu_open,
    .release        = gen_te_hu_release,
	.read			= gen_te_hu_read,
};

static int te_hu_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	printk_log("%s i2c driver and device was matched!\n", __func__);

	g_te_hu_dev = kzalloc(sizeof(struct te_hu_dev), GFP_KERNEL);
	if (!g_te_hu_dev) {
		printk_err("te_hu malloc error.\n");
		ret = -1;
		goto end;
	}

	memcpy(g_te_hu_dev, &g_te_hu_init, sizeof(struct te_hu_dev));
	
	g_te_hu_dev->misc_dev.minor 	= MISC_DYNAMIC_MINOR;
    g_te_hu_dev->misc_dev.name 		= TE_HU_DEV_NAME;
    g_te_hu_dev->misc_dev.fops  	= &gen_te_hu_fops;
	
    ret = misc_register(&g_te_hu_dev->misc_dev);
    if(unlikely(ret < 0)) {
        printk_err("register misc fail\n");
		ret = -1;
		goto end;
    }
	
	mutex_init(&g_te_hu_dev->buf_lock);

	switch (g_te_hu_dev->drv_chip) {
		case CHIP_GXHT3X:
			g_te_hu_dev->te_hu_open  = gxht3x_open;
			g_te_hu_dev->te_hu_close = gxht3x_close;
			g_te_hu_dev->te_hu_read  = gxht3x_read;
			g_te_hu_dev->chip_conf.gxht3x_chip.client = client; 
			break;
		default:
			break;
	}
end: 
	return ret;	
}

static int te_hu_remove(struct i2c_client *client)
{
	misc_deregister(&g_te_hu_dev->misc_dev);

	kfree(g_te_hu_dev);
	g_te_hu_dev = NULL;

	return 0;
}

static const struct i2c_device_id te_hu_id[] = {
	{"ingenic,t40-i2c", 0},
	{}
};
 
static const struct of_device_id te_hu_of_device[] = {
	{.compatible = "ingenic,t40-i2c"},
	{ /* sentinel */ }
};
 
static struct i2c_driver te_hu_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "t40-i2c",
		.of_match_table = te_hu_of_device,
	},
	.id_table = te_hu_id,
	.probe = te_hu_probe,
	.remove = te_hu_remove,
};

int pg_te_hu_init(void)
{
	int ret = 0;
	
	ret = i2c_add_driver(&te_hu_driver);
	
    printk_log("%s module is installed\n", __func__);
 
    return ret;
}
 
void pg_te_hu_exit(void)
{	
	i2c_del_driver(&te_hu_driver);
	
    printk_log("%s module is removed.\n", __func__);
	
    return;
}
