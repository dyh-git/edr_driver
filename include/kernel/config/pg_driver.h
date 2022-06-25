#ifndef __PG_DRIVER_H__
#define __PG_DRIVER_H__

#include "pg_typedefs.h"

typedef enum{
	PG_MOTOR_MODULE,
	PG_TE_HU_MODULE,
	PG_ACC_GYRO_MODULE,
	PG_FUEL_GAUGE_MODULE,
	PG_MODULE_NR,
}pg_driver_module_e;

extern int pg_motor_init(void) __attribute__((weak));
extern void pg_motor_exit(void) __attribute__((weak));

extern int pg_te_hu_init(void) __attribute__((weak));
extern void pg_te_hu_exit(void) __attribute__((weak));

extern int pg_acc_gyro_init(void) __attribute__((weak));
extern void pg_acc_gyro_exit(void) __attribute__((weak));

extern int pg_fuel_gauge_init(void) __attribute__((weak));
extern void pg_fuel_gauge_exit(void) __attribute__((weak));

int pg_motor_init(void)
{
	printk_log("pg_motor_init null\n");
	return 0;
}
void pg_motor_exit(void)	{return;}

int pg_te_hu_init(void)
{
	printk_log("pg_te_hu_init null\n");
	return 0;
}
void pg_te_hu_exit(void)	{return;}

int pg_acc_gyro_init(void)
{
	printk_log("pg_acc_gyro_init null\n");
	return 0;
}
void pg_acc_gyro_exit(void)	{return;}

int pg_fuel_gauge_init(void)
{
	printk_log("pg_fuel_gauge_init null\n");
	return 0;
}
void pg_fuel_gauge_exit(void)	{return;}

#endif


