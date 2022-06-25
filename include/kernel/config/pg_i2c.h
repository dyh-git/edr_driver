#ifndef __PG_I2C_H__
#define __PG_I2C_H__

#include "pg_typedefs.h"

typedef enum{
	I2C_MODULE,
	I2C_EMULATE,
}gen_i2c_oper_mode_e;

int i2c_emulate_write_data(void *handle, const reg_t *s_reg);
int i2c_emulate_read_data(void *handle, const reg_t *s_reg, const buf_t *r_buf);
int i2c_module_write_data(void *handle, const reg_t *s_reg);
int i2c_module_read_data(void *handle, const reg_t *s_reg, const buf_t *r_buf);

#endif