/*
* pg_driver.c - edr
*
* Copyright (C) 2020 PanGu Technology Corp.
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <config/pg_driver.h>

extern uint8_t g_pg_dmodule[];
extern uint8_t g_pg_dmodule_len;

static int __init pg_driver_init(void)
{
	uint8_t	i 	= 0;
	int	ret 	= 0;
	
	if (g_pg_dmodule_len > PG_MODULE_NR) {
		printk_err("config error.need less than total drivers\n");
		return -1;
	}

	for (i = 0; i < g_pg_dmodule_len; i++) {
		switch(g_pg_dmodule[i]) {
			case PG_MOTOR_MODULE:
				ret = pg_motor_init();
				if (ret < 0)
					printk_err("module %d failed.\n", g_pg_dmodule[i]);
				break;
			case PG_TE_HU_MODULE:
				ret = pg_te_hu_init();
				if (ret < 0)
					printk_err("module %d failed.\n", g_pg_dmodule[i]);				
				break;
			case PG_ACC_GYRO_MODULE:
				ret = pg_acc_gyro_init();
				if (ret < 0)
					printk_err("module %d failed.\n", g_pg_dmodule[i]);				
				break;
			case PG_FUEL_GAUGE_MODULE:
				ret = pg_fuel_gauge_init();
				if (ret < 0)
					printk_err("module %d failed.\n", g_pg_dmodule[i]);				
				break;
			default:
				
				break;
		}
	}
	
	return ret;
}

static void __exit pg_driver_exit(void)
{
	uint8_t	i = 0;

	if (g_pg_dmodule_len > PG_MODULE_NR) {
		printk_err("config error.need less than total drivers\n");
		
		return;
	}
	
	for (i = 0; i < g_pg_dmodule_len; i++) {
		switch(g_pg_dmodule[i]) {
			case PG_MOTOR_MODULE:
				pg_motor_exit();
				break;
			case PG_TE_HU_MODULE:
				pg_te_hu_exit();
				break;
			case PG_ACC_GYRO_MODULE:
				pg_acc_gyro_exit();
				break;
			case PG_FUEL_GAUGE_MODULE:
				pg_fuel_gauge_exit();
				break;				
			default:
				
				break;
		}		
	}

    return;
}

module_init(pg_driver_init);
module_exit(pg_driver_exit);

MODULE_AUTHOR("PanGu");
MODULE_DESCRIPTION("PanGu Driver");
MODULE_LICENSE("GPL");
