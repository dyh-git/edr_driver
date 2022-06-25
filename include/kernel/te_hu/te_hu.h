#ifndef __TE_HU_H__
#define __TE_HU_H__

#include "gxht3x.h"

#define TE_HU_DEV_NAME		"pg_te_hu"

typedef enum te_hu_chip_type{
	CHIP_GXHT3X = 1,
}te_hu_chip_type_e;

typedef union te_hu_chip {
	gxht3x_attri_s gxht3x_chip;
}te_hu_chip_u;

typedef struct te_hu_dev {
	struct miscdevice 	misc_dev;
	struct mutex		buf_lock;
	te_hu_chip_type_e 	drv_chip;
	
	te_hu_chip_u chip_conf;
	
	int (*te_hu_open)(void *chip);
	int (*te_hu_close)(void *chip);
	int (*te_hu_read)(void *chip, char *buf, size_t count);
}te_hu_dev_t;

#endif