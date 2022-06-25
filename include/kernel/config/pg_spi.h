#ifndef __PG_SPI_H__
#define __PG_SPI_H__

#include "pg_typedefs.h"

typedef enum{
	SPI_MODULE,
	SPI_EMULATE,
}gen_spi_oper_mode_e;

int spi_emulate_write_data(void *handle, const reg_t *s_reg);
int spi_emulate_read_data(void *handle, const reg_t *s_reg, const buf_t *r_buf);
int spi_module_write_data(void *handle, const reg_t *s_reg);
int spi_module_read_data(void *handle, const reg_t *s_reg, const buf_t *r_buf);

#endif