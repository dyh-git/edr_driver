#ifndef __CW2218_H__
#define __CW2218_H__

#include <linux/delay.h>
#include <config/pg_i2c.h>

#define REG_CHIP_ID             0x00
#define REG_VCELL_H             0x02
#define REG_VCELL_L             0x03
#define REG_SOC_INT             0x04
#define REG_SOC_DECIMAL         0x05
#define REG_TEMP                0x06
#define REG_MODE_CONFIG         0x08
#define REG_GPIO_CONFIG         0x0A
#define REG_SOC_ALERT           0x0B
#define REG_TEMP_MAX            0x0C
#define REG_TEMP_MIN            0x0D
#define REG_CURRENT_H           0x0E
#define REG_CURRENT_L           0x0F
#define REG_T_HOST_H            0xA0
#define REG_T_HOST_L            0xA1
#define REG_USER_CONF           0xA2
#define REG_CYCLE_H             0xA4
#define REG_CYCLE_L             0xA5
#define REG_SOH                 0xA6
#define REG_IC_STATE            0xA7
#define REG_STB_CUR_H           0xA8
#define REG_STB_CUR_L           0xA9
#define REG_FW_VERSION          0xAB
#define REG_BAT_PROFILE         0x10

#define CONFIG_MODE_RESTART     0x30
#define CONFIG_MODE_ACTIVE      0x00
#define CONFIG_UPDATE_FLG       0x80
#define CONFIG_MODE_SLEEP       0xF0
#define IC_VCHIP_ID             0xA0
#define IC_READY_MARK           0x0C
#define IC_TEMP_READY           0x08
#define IC_VOL_CUR_READY        0x04

#define SIZE_OF_PROFILE         80

#define RSENSE 					10

#define GPIO_SOC_IRQ_VALUE      0x0    /* 0x7F */
#define CW_SLEEP_COUNTS         50

#define UI_FULL     			100

enum cw221x_data_type{
	CW221X_TYPE_VOL,
	CW221X_TYPE_CAP,
	CW221X_TYPE_TEMP,
	CW221X_TYPE_CURRENT,
	CW221X_TYPE_CYCLE_CNT,
	CW221X_TYPE_SOH,
};

typedef struct cw221x_attri{
	gen_i2c_oper_mode_e oper_mode;
	struct i2c_client *client;
	struct i2c_emulate *emu_client;
	
	void *handle;

	int (*write_data)(void *handle, const reg_t *s_reg);
	int (*read_data)(void *handle, const reg_t *s_reg, const buf_t *r_buf);
}cw221x_attri_s;

int cw221x_open(void *chip);
int cw221x_close(void *chip);
int cw221x_read(void *chip, char *buf, size_t count);

#endif