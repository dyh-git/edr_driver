#include <config/pg_typedefs.h>
#include <fuel_gauge/cw2218.h>

/**************************global**********************************/
static uint8_t config_profile_info[SIZE_OF_PROFILE] = {
	0x5A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xB9,0xB6,0xC2,0xBD,0xC4,0xC1,0x95,0x5C,
	0x2C,0xFF,0xFF,0xE1,0xBF,0x7F,0x6A,0x5B,
	0x51,0x4C,0x46,0x84,0xC2,0xD9,0x9E,0xD5,
	0xCF,0xCE,0xCD,0xCA,0xC9,0xB1,0xE0,0xAE,
	0xBD,0xC6,0xAB,0x98,0x8C,0x83,0x7C,0x6C,
	0x63,0x65,0x80,0x91,0xA2,0x73,0x61,0x53,
	0x00,0x00,0x57,0x10,0x00,0x40,0xF6,0x00,
	0x00,0x00,0x64,0x1F,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF4,
};

static int cw221x_send_data(cw221x_attri_s *cw221x, const reg_t *s_reg)
{
	int ret = 0;

	if (!(cw221x->write_data)) {
		printk_err("write_data function error.\n");
		
		return -1;
	}
	
	ret = cw221x->write_data(cw221x->handle, s_reg);
	if (ret < 0)
		printk_err("write data error.\n");
	
	return ret;	
}

static int cw221x_rcv_data(cw221x_attri_s *cw221x, const reg_t *s_reg, const buf_t *r_buf)
{
	int ret = 0;

	if (!(cw221x->read_data)) {
		printk_err("read_data function error.\n");
		
		return -1;
	}
		
	ret = cw221x->read_data(cw221x->handle, s_reg, r_buf);
	if (ret < 0)
		printk_err("read data error.\n");
	
	return ret;	
}

static int cw221x_write_reg(cw221x_attri_s *cw221x, uint8_t reg_addr, uint8_t data)
{
	int ret 			= 0;
	uint8_t msg_data[2] = {0x00};
	reg_t s_reg;
	
	msg_data[0] = reg_addr;
	msg_data[1] = data;
	s_reg.buf = msg_data;
	s_reg.len = 2;
	
	ret = cw221x_send_data(cw221x, &s_reg);
	if (ret < 0) {
		printk_err("cw221x read data error.\n");
		return -1;
	}

	return ret;
}

static int cw221x_read_reg(cw221x_attri_s *cw221x, uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
	int ret = 0;
	reg_t s_reg;
	buf_t r_buf;
	
	s_reg.buf = &reg_addr;
	s_reg.len = 1;
	r_buf.buf = buf;
	r_buf.len = len;
	
	ret = cw221x_rcv_data(cw221x, &s_reg, &r_buf);
	if (ret < 0) {
		printk_err("cw221x read data error.\n");
		return -1;
	}
	
	return ret;
}

static int cw221x_sleep(cw221x_attri_s *cw221x)
{
	int ret			= 0;
	uint8_t reg_val = CONFIG_MODE_RESTART;

	ret = cw221x_read_reg(cw221x, REG_MODE_CONFIG, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	mdelay(20); /* Here delay must >= 20 ms */

	reg_val = CONFIG_MODE_SLEEP;
	ret = cw221x_read_reg(cw221x, REG_MODE_CONFIG, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	mdelay(10); 

	return 0;
}

static int cw221x_write_profile(cw221x_attri_s *cw221x, uint8_t const *buf)
{
	int ret		= 0;
	uint8_t i	= 0;

	for (i = 0; i < SIZE_OF_PROFILE; i++) {
		ret = cw221x_write_reg(cw221x, REG_BAT_PROFILE + i, buf[i]);
		if(ret < 0) {
			printk_err("cw221x error i2c, times=%d.\n", i);
			return -1;
		}
	}

	return 0;
}

static int cw221x_active(cw221x_attri_s *cw221x)
{
	int ret = 0;

	ret = cw221x_write_reg(cw221x, REG_MODE_CONFIG, CONFIG_MODE_RESTART);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	mdelay(20); /* Here delay must >= 20 ms */

	ret = cw221x_write_reg(cw221x, REG_MODE_CONFIG, CONFIG_MODE_ACTIVE);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	mdelay(10); 

	return 0;
}

static int cw221x_get_chip_id(cw221x_attri_s *cw221x, int *chip_id)
{
	int ret			= 0;
	uint8_t reg_val = 0;
	
	ret = cw221x_read_reg(cw221x, REG_CHIP_ID, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}

	*chip_id = reg_val;

	return 0;
}

static int cw221x_get_state(cw221x_attri_s *cw221x)
{
	int ret			= 0;
	uint8_t reg_val = 0;
	uint8_t i		= 0;
	int reg_profile = 0;
	
	ret = cw221x_read_reg(cw221x, REG_MODE_CONFIG, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}

	if (reg_val != CONFIG_MODE_ACTIVE) {
		printk_err("cw221x not active.\n");
		return -1;
	}
	
	ret = cw221x_read_reg(cw221x, REG_SOC_ALERT, &reg_val, 1);
	if (ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}

	if (0x00 == (reg_val & CONFIG_UPDATE_FLG)) {
		printk_err("cw221x profile not ready.\n");
		return -1;
	}
	
	for (i = 0; i < SIZE_OF_PROFILE; i++) {
		ret = cw221x_read_reg(cw221x, (REG_BAT_PROFILE + i), &reg_val, 1);
		if (ret < 0) {
			printk_err("cw221x error i2c.\n");
			return -1;
		}
		reg_profile = REG_BAT_PROFILE + i;
		if (config_profile_info[i] != reg_val)
			break;
	}
	if (i != SIZE_OF_PROFILE) {
		printk_err("cw221x profile need update.\n");
		return -1;
	}
	
	return 0;
}

/*CW221X update profile function, Often called during initialization*/
static int cw221x_config_start_ic(cw221x_attri_s *cw221x)
{
	int ret			 = 0;
	uint8_t reg_val	 = 0;

	ret = cw221x_sleep(cw221x);
	if (ret < 0)
		return ret;

	/* update new battery info */
	ret = cw221x_write_profile(cw221x, config_profile_info);
	if (ret < 0)
		return ret;

	/* set UPDATE_FLAG AND SOC INTTERRUP VALUE*/
	reg_val = CONFIG_UPDATE_FLG | GPIO_SOC_IRQ_VALUE;   
	ret = cw221x_write_reg(cw221x, REG_SOC_ALERT, reg_val);
	if (ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}

	/*close all interruptes*/
	reg_val = 0; 
	ret = cw221x_write_reg(cw221x, REG_GPIO_CONFIG, reg_val); 
	if (ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}

	ret = cw221x_active(cw221x);
	if (ret < 0) 
		return ret;
	
	return 0;
}

static int cw221x_init(cw221x_attri_s *cw221x)
{
	int ret		= 0;
	int chip_id = 0;

	ret = cw221x_get_chip_id(cw221x, &chip_id);
	if(ret < 0){
		return -1;
	}

	if(chip_id != IC_VCHIP_ID) {
		printk_err("cw221x error chip id.\n");
		return -1;
	}

	ret = cw221x_get_state(cw221x);
	if (ret < 0)
		return -1;

	ret = cw221x_config_start_ic(cw221x);
	if (ret < 0)
		return -1;

	return 0;
}

static int cw221x_bat_init(cw221x_attri_s *cw221x)
{
	int ret		 = 0;
	uint8_t loop = 0;
	
	ret = cw221x_init(cw221x);
	while((loop++ < 3) && (ret != 0)) {
		ret = cw221x_init(cw221x);
	}
	
	return ret;	
}

static int cw221x_get_vol(cw221x_attri_s *cw221x, uint16_t *lp_vol)
{
	int ret 		= 0;
	uint8_t reg_val = 0;
	uint16_t temp_val_buff 	= 0;
	uint16_t ad_value 		= 0;

	ret = cw221x_read_reg(cw221x, REG_IC_STATE, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	if((reg_val & IC_READY_MARK) < IC_VOL_CUR_READY) {
		printk_log("voltage not ready.\n");
		*lp_vol = 4000;

		return 0;
	}

	ret = cw221x_read_reg(cw221x, REG_VCELL_H, (uint8_t *)&temp_val_buff, 2);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	
	ad_value = temp_val_buff * 5 / 16;
	*lp_vol = ad_value;
	
	return 0; 
}

static int cw221x_get_capacity(cw221x_attri_s *cw221x, int *lp_uisoc)
{
	int ret 		= 0;
	uint8_t reg_val = 0;
	uint8_t count 	= 0;
	uint16_t temp_val_buff = 0;
	int soc 		= 0;
	int soc_decimal = 0;
	int remainder 	= 0;
	int ui_soc 		= 0;

	while (1) {
		ret = cw221x_read_reg(cw221x, REG_IC_STATE, &reg_val, 1);
		if(ret < 0) {
			printk_err("cw221x error i2c.\n");
			return -1;
		}
		if (IC_READY_MARK == (reg_val & IC_READY_MARK))
			break;

		count++;

		if (count >= CW_SLEEP_COUNTS) {
			cw221x_sleep(cw221x);
			printk_err("cw221x error time out.\n");
			return -1;
		}
		mdelay(100); /*sleep 100 ms must*/
	}

	ret = cw221x_read_reg(cw221x, REG_SOC_INT, (uint8_t *)&temp_val_buff, 2);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	
	soc = temp_val_buff >> 8;
	soc_decimal = temp_val_buff & 0xFF;

 	ui_soc = (((uint32_t)soc * 256 + soc_decimal) * 100)/ (UI_FULL * 256);
	remainder = ((((uint32_t)soc * 256 + soc_decimal) * 100 * 100) / (UI_FULL * 256)) % 100;
	
	if(ui_soc >= 100) {
		ui_soc = 100;
	}
	*lp_uisoc = ui_soc;
	
	return 0;
}

static int cw221x_get_temp(cw221x_attri_s *cw221x, int *lp_temp)
{
	int ret 		= 0;
	uint8_t reg_val = 0;
	int temp 		= 0;

	ret = cw221x_read_reg(cw221x, REG_IC_STATE, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	if((reg_val & IC_READY_MARK) < IC_TEMP_READY) {
		printk_log("Temp not ready.\n");
		*lp_temp = 250;

		return 0;
	}

	ret = cw221x_read_reg(cw221x, REG_TEMP, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}

	temp = (int)reg_val * 10 / 2 - 400;
	*lp_temp = temp;
	
	return 0;
}

static long get_complement_code(uint16_t raw_code)
{
	long complement_code = 0;
	int dir 			 = 0;

	if (0 != (raw_code & 0x8000)){
		dir = -1;
		raw_code =  (~raw_code) + 1;
	}
	else{
		dir = 1;
	}

	complement_code = (long)raw_code * dir;

	return complement_code;
}

static int cw221x_get_current(cw221x_attri_s *cw221x, long *lp_current)
{
	int ret 		= 0;
	uint8_t reg_val = 0;
	uint16_t temp_val_buff = 0;
	long current 	= 0;

	ret = cw221x_read_reg(cw221x, REG_IC_STATE, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	if((reg_val & IC_READY_MARK) < IC_VOL_CUR_READY){
		printk_log("current not ready.\n");
		*lp_current = 0;
		return 0;
	}

	ret = cw221x_read_reg(cw221x, REG_CURRENT_H, (uint8_t *)&temp_val_buff, 2);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}
	
	current = get_complement_code(temp_val_buff);
	current = current * 763 / 2 / RSENSE / 100;
	*lp_current = current;

	return 0; 
}

static int cw221x_get_cycle_count(cw221x_attri_s *cw221x, int *lp_count)
{
	int ret 		= 0;
	uint16_t temp_val_buff = 0;
	int cycle_buff 	= 0;

	ret = cw221x_read_reg(cw221x, REG_CYCLE_H, (uint8_t *)&temp_val_buff, 2);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}

	cycle_buff = temp_val_buff / 16;
	*lp_count = cycle_buff;

	return 0;	
}

static int cw221x_get_soh(cw221x_attri_s *cw221x, int *lp_soh)
{
	int ret 	= 0;
	uint8_t reg_val = 0;
	uint8_t soh = 0;

	ret = cw221x_read_reg(cw221x, REG_SOH, &reg_val, 1);
	if(ret < 0) {
		printk_err("cw221x error i2c.\n");
		return -1;
	}

	soh = reg_val;
	*lp_soh = soh;

	return 0;
}

static void cw221x_init_handle(cw221x_attri_s *cw221x)
{	
	if (I2C_EMULATE == cw221x->oper_mode) {
		cw221x->write_data = i2c_emulate_write_data;
		cw221x->read_data 	= i2c_emulate_read_data;
	} else {
		cw221x->write_data = i2c_module_write_data;
	}   cw221x->read_data 	= i2c_module_read_data;
	
	if (cw221x->client) {
		cw221x->handle = (void *)cw221x->client;
	} else {
		cw221x->handle = (void *)cw221x->emu_client;
	}

	return;
}

int cw221x_open(void *chip)
{
	int ret = 0;
	
	cw221x_attri_s *cw221x_one = (cw221x_attri_s *)chip;
	
	cw221x_init_handle(cw221x_one);
	ret = cw221x_bat_init(cw221x_one);
	if (ret < 0) {
		printk("cw221x_bat_init error.\n");
		
		return -1;
	}
	
	return 0;
}

int cw221x_close(void *chip)
{
	int ret = 0;
//	cw221x_attri_s *cw221x_one = (cw221x_attri_s *)chip;
	
	return ret;
}

int cw221x_read(void *chip, char *buf, size_t count)
{
	int ret			= 0;
	uint16_t vol 	= 0;
	int capacity 	= 0;
	int temp		= 0;
	long current	= 0;
	int cycle_count = 0;
	int soh			= 0;
	cw221x_attri_s *cw221x_one = (cw221x_attri_s *)chip;
	uint8_t data_type = buf[0];

	switch (data_type) {
		case CW221X_TYPE_VOL:
			ret =  cw221x_get_vol(cw221x_one, &vol);
			if (ret < 0) {
				printk_err("cw221x get vol error.\n");
				return -1;
			}
			buf[0] = (uint8_t)(vol);
			buf[1] = (uint8_t)(vol >> 8);
			break;
		case CW221X_TYPE_CAP:
			ret =  cw221x_get_capacity(cw221x_one, &capacity);
			if (ret < 0) {
				printk_err("cw221x get capacity error.\n");
				return -1;
			}
			buf[0] = (uint8_t)(capacity);
			buf[1] = (uint8_t)(capacity >> 8);
			buf[2] = (uint8_t)(capacity >> 16);
			buf[3] = (uint8_t)(capacity >> 24);		
			break;
		case CW221X_TYPE_TEMP:
			ret =  cw221x_get_temp(cw221x_one, &temp);
			if (ret < 0) {
				printk_err("cw221x get capacity error.\n");
				return -1;
			}
			buf[0] = (uint8_t)(temp);
			buf[1] = (uint8_t)(temp >> 8);
			buf[2] = (uint8_t)(temp >> 16);
			buf[3] = (uint8_t)(temp >> 24);				
			break;
		case CW221X_TYPE_CURRENT:
			ret =  cw221x_get_current(cw221x_one, &current);
			if (ret < 0) {
				printk_err("cw221x get current error.\n");
				return -1;
			}
			buf[0] = (uint8_t)(current);
			buf[1] = (uint8_t)(current >> 8);
			buf[2] = (uint8_t)(current >> 16);
			buf[3] = (uint8_t)(current >> 24);	
			break;
		case CW221X_TYPE_CYCLE_CNT:
			ret =  cw221x_get_cycle_count(cw221x_one, &cycle_count);
			if (ret < 0) {
				printk_err("cw221x get cycle_count error.\n");
				return -1;
			}
			buf[0] = (uint8_t)(cycle_count);
			buf[1] = (uint8_t)(cycle_count >> 8);
			buf[2] = (uint8_t)(cycle_count >> 16);
			buf[3] = (uint8_t)(cycle_count >> 24);	
			break;
		case CW221X_TYPE_SOH:
			ret =  cw221x_get_soh(cw221x_one, &soh);
			if (ret < 0) {
				printk_err("cw221x get soh error.\n");
				return -1;
			}
			buf[0] = (uint8_t)(soh);
			buf[1] = (uint8_t)(soh >> 8);
			buf[2] = (uint8_t)(soh >> 16);
			buf[3] = (uint8_t)(soh >> 24);	
			break;
		default:
			printk_err("which data do you want.\n");
			return -1;
	}
	
	return 0;
}
