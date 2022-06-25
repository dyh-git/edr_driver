#ifndef __FUEL_GAUGE_H__
#define __FUEL_GAUGE_H__

#include "cw2218.h"

#define FUEL_GAUGE_DEV_NAME	"pg_fuel_gauge"

typedef enum fuel_gauge_drv_chip {
	CHIP_CW221X = 1,
}fuel_gauge_drv_chip_e;

typedef union fuel_gauge_chip {
	cw221x_attri_s cw221x_chip;
}fuel_gauge_chip_u;

typedef struct fuel_gauge_dev {
	struct miscdevice 	misc_dev;
	struct mutex		buf_lock;
	
	fuel_gauge_drv_chip_e drv_chip;
	fuel_gauge_chip_u chip_conf;
	
	int (*fuel_gauge_open)(void *chip);
	int (*fuel_gauge_close)(void *chip);
	int (*fuel_gauge_read)(void *chip, char *buf, size_t count);
}fuel_gauge_dev_s;

#endif