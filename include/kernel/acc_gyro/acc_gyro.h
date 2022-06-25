#ifndef __ACC_GYRO_H__
#define __ACC_GYRO_H__

#include "qmi8658a.h"

#define ACC_GYRO_DEV_NAME	"pg_acc_gyro"

typedef enum acc_gyro_drv_chip {
	CHIP_QMI8658 = 1,
}acc_gyro_drv_chip_e;

typedef union acc_gyro_chip {
	qmi8658_attri_s qmi8658_chip;
}acc_gyro_chip_u;

typedef struct acc_gyro_dev {
	struct miscdevice 	misc_dev;
	struct mutex		buf_lock;
	
	acc_gyro_drv_chip_e	drv_chip;
	acc_gyro_chip_u chip_conf;
	
	int (*acc_gyro_open)(void *chip);
	int (*acc_gyro_close)(void *chip);
	int (*acc_gyro_read)(void *chip, char *buf, size_t count);
}acc_gyro_dev_s;

#endif