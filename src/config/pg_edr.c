#include <motor/motor.h>
#include <te_hu/te_hu.h>
#include <fuel_gauge/fuel_gauge.h>
#include <acc_gyro/acc_gyro.h>
#include <config/pg_driver.h>

uint8_t g_pg_dmodule[] = {
	PG_TE_HU_MODULE,
	PG_ACC_GYRO_MODULE,
};

uint8_t g_pg_dmodule_len = sizeof(g_pg_dmodule)/sizeof(int);

struct te_hu_dev g_te_hu_init = {
	.drv_chip = CHIP_GXHT3X,
	.chip_conf = {
		.gxht3x_chip = {
			.oper_mode 	= I2C_MODULE,
			.mode		= MODE_POLLING,
			.repeat		= REPEATAB_MEDIUM,
			.timeout 	= UN_MG_DPS_MODE,
		},
	},
};

struct acc_gyro_dev g_acc_gyro_init = {
	.drv_chip = CHIP_QMI8658,
	.chip_conf = {
		.qmi8658_chip = {
			.oper_mode = I2C_MODULE,
			.mg_dps_mode = UN_MG_DPS_MODE,
		},
	},
};

struct fuel_gauge_dev g_fuel_gauge_init = {
	.drv_chip = CHIP_CW221X,
	.chip_conf = {
		.cw221x_chip = {
			.oper_mode = I2C_MODULE,
		},
	},
};

struct motor_dev g_motor_init = {
	.drv_chip = CHIP_TMI8150,
	.chip_conf = {
		.tmi8150_chip = {
			.oper_mode = SPI_MODULE,
			.base_period   = 100000,
			.x_axis_enable = 1,
			.y_axis_enable = 0,

			.xaxis_step_max = 1000,
			.yaxis_step_max = 0,
		},
	},
};