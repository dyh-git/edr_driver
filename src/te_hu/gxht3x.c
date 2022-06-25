#include <config/pg_typedefs.h>
#include <te_hu/gxht3x.h>

static int gxht3x_chk_crc(uint8_t *data, uint8_t num, uint8_t crc_sum)
{
	uint8_t i 		= 0;
	uint8_t bit		= 0;
	uint8_t crc	 	= 0xFF;
	
	for (i = 0; i < num; i++) {
		crc ^= data[i];
		for (bit = 8; bit > 0; --bit) {
			if(crc & 0x80)
				crc = (crc << 1) ^ POLYNOMIAL;
			else
				crc = (crc << 1);
		}		
	}
	
	if (crc != crc_sum)
		return -1;

	return 0;
}

static int gxht3x_calc_temp(int raw_value)
{
	int res = 0;

	raw_value = 100 * raw_value;

	res = 175 * raw_value / 65535 - 45;

	return res; 
}

static uint32_t gxht3x_calc_humi(uint32_t raw_value)
{
	uint32_t res = 0;
	
	raw_value = 100 * raw_value;

	res = 100 * raw_value / 65535;

	return res;
}

static int gxht3x_send_cmd(gxht3x_attri_s *gxht3x, uint16_t cmd)
{
	int ret 			= 0;
	uint8_t msg_data[2] = {0x00};
	reg_t s_reg;

	msg_data[0] = (uint8_t)cmd;
	msg_data[1] = (uint8_t)(cmd >> 8);
	
	s_reg.len 	= 2;
	s_reg.buf	= msg_data;
	
	if (!(gxht3x->write_data)) {
		printk_err("write_data function error.\n");
		
		return -1;
	}
	
	ret = gxht3x->write_data(gxht3x->handle, &s_reg);
	if (ret < 0)
		printk_err("write cmd error.\n");

	return ret;
}

int gxht3x_rcv_data_normal(gxht3x_attri_s *gxht3x, const reg_t *s_reg, const buf_t *r_buf)
{
	int ret = 0;

	if (!(gxht3x->read_data)) {
		printk_err("read_data function error.\n");
		
		return -1;
	}	
	ret = gxht3x->read_data(gxht3x->handle, s_reg, r_buf);
	if (ret < 0)
		printk_err("read data error.\n");
	
	return ret;	
}

static int gxht3x_recv_serial_data(gxht3x_attri_s *gxht3x)
{
	int ret 				= 0;
	uint32_t serial_num		= 0;
	uint8_t cmd_data[2]		= {0x00};
	uint8_t serial_data[6] 	= {0x00};
	reg_t send_buf;
	buf_t recv_buf;
	
	memset(&send_buf, 0x00, sizeof(reg_t));
	cmd_data[0]	 = (uint8_t)CMD_READ_SERIALNBR;
	cmd_data[1]	 = (uint8_t)(CMD_READ_SERIALNBR >> 8);
	send_buf.buf = cmd_data;
	send_buf.len = sizeof(cmd_data)/sizeof(cmd_data[0]);

	memset(serial_data, 0x00, sizeof(serial_data)/sizeof(serial_data[0]));	
	memset(&recv_buf, 0x00, sizeof(buf_t));
	recv_buf.buf = serial_data;
	recv_buf.len = sizeof(serial_data)/sizeof(serial_data[0]);
	
	ret = gxht3x_rcv_data_normal(gxht3x, &send_buf, &recv_buf);
	if (ret < 0) {
		printk_err("recv serial num error.\n");
		
		return -1;
	}	
	
	printk_dbg("serial data: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", serial_data[0], serial_data[1], serial_data[2], serial_data[3], serial_data[4], serial_data[5]);
	ret = gxht3x_chk_crc(serial_data, 2, serial_data[2]);
	if (ret) {
		printk_err("serial data1 crc check error.\n");
		
		return -1;
	}

	ret = gxht3x_chk_crc(&serial_data[3], 2, serial_data[5]);
	if (ret) {
		printk_err("serial data2 crc check error.\n");
		
		return -1;
	}
	
	serial_num = (((serial_data[0] << 8) + serial_data[1]) << 16) + (serial_data[3] << 8) + serial_data[4];
	printk_log("serial number:0x%x", serial_num);
	
	return 0;
}

static int gxht3x_get_temp_humi_clkstretch(gxht3x_attri_s *gxht3x, int *temp, uint32_t *humi)
{
	int ret 				= 0;
	uint16_t reg_addr 		= 0;
	uint8_t cmd_data[2]		= {0x00};
	uint8_t te_hu_data[6] 	= {0x00};
	reg_t send_buf;
	buf_t recv_buf;
	
	switch (gxht3x->repeat) {
		case REPEATAB_HIGH:
			reg_addr = CMD_MEAS_CLOCKSTR_H;
			break;
		case REPEATAB_MEDIUM:
			reg_addr = CMD_MEAS_CLOCKSTR_M;
			break;
		case REPEATAB_LOW:
			reg_addr = CMD_MEAS_CLOCKSTR_L;
			break;
		default:
			reg_addr = CMD_MEAS_CLOCKSTR_L;
			break;
	}
	
	ret = gxht3x_send_cmd(gxht3x, reg_addr);
	if (ret < 0) {
		printk_err("send %d cmd error.\n", reg_addr);
		
		return -1;
	}
	
	msleep(1);

	memset(&send_buf, 0x00, sizeof(reg_t));
	send_buf.buf = cmd_data;
	send_buf.len = 0;

	memset(te_hu_data, 0x00, sizeof(te_hu_data)/sizeof(te_hu_data[0]));	
	memset(&recv_buf, 0x00, sizeof(buf_t));
	recv_buf.buf = te_hu_data;
	recv_buf.len = sizeof(te_hu_data)/sizeof(te_hu_data[0]);
	
	ret = gxht3x_rcv_data_normal(gxht3x, &send_buf, &recv_buf);
	if (ret < 0) {
		printk_err("recv temp and hum datas error in stretch mode.\n");
		
		return -1;
	}	

	ret = gxht3x_chk_crc(te_hu_data, 2, te_hu_data[2]);
	if (ret < 0) {
		printk_err("temp data crc check error in strech mode.\n");
		
		return -1;
	}
	ret = gxht3x_chk_crc(&te_hu_data[3], 2, te_hu_data[5]);
	if (ret < 0) {
		printk_err("humi data crc check error in strech mode.\n");
		
		return -1;
	}
	
	*temp = gxht3x_calc_temp((int)((te_hu_data[0] << 8) | te_hu_data[1]));
	*humi = gxht3x_calc_humi((uint32_t)((te_hu_data[3] << 8) | te_hu_data[4]));
	
	return 0;
}

static int gxht3x_get_temp_humi_polling(gxht3x_attri_s *gxht3x, int *temp, uint32_t *humi)
{
	int ret 				= 0;
	uint8_t try_times		= 10;
	uint16_t reg_addr 		= 0;
	uint8_t cmd_data[2]		= {0x00};
	uint8_t te_hu_data[6] 	= {0x00};
	reg_t send_buf;
	buf_t recv_buf;
	
	switch (gxht3x->repeat) {
		case REPEATAB_HIGH:
			reg_addr = CMD_MEAS_POLLING_H;
			break;
		case REPEATAB_MEDIUM:
			reg_addr = CMD_MEAS_POLLING_M;
			break;
		case REPEATAB_LOW:
			reg_addr = CMD_MEAS_POLLING_L;
			break;
		default:
			reg_addr = CMD_MEAS_POLLING_L;
			break;
	}

	ret = gxht3x_send_cmd(gxht3x, reg_addr);
	if (ret < 0) {
		printk_err("send %d cmd error.\n", reg_addr);
		
		return -1;
	}
	
	msleep(1);
	
	memset(&send_buf, 0x00, sizeof(reg_t));
	send_buf.buf = cmd_data;
	send_buf.len = 0;

	memset(&recv_buf, 0x00, sizeof(buf_t));
	recv_buf.buf = te_hu_data;
	recv_buf.len = 0;

	ret = gxht3x_rcv_data_normal(gxht3x, &send_buf, &recv_buf);
	if (ret < 0) {
		printk_err("step1: recv temp and hum datas error in polling mode.\n");
		
		return -1;
	}

	while (try_times) {
		memset(&send_buf, 0x00, sizeof(reg_t));
		send_buf.buf = cmd_data;
		send_buf.len = 0;

		memset(te_hu_data, 0x00, sizeof(te_hu_data)/sizeof(te_hu_data[0]));	
		memset(&recv_buf, 0x00, sizeof(buf_t));
		recv_buf.buf = te_hu_data;
		recv_buf.len = sizeof(te_hu_data)/sizeof(te_hu_data[0]);

		ret = gxht3x_rcv_data_normal(gxht3x, &send_buf, &recv_buf);
		if (ret < 0) {
			msleep(1);
			try_times--;
		} else {
			break;
		}
	}

	if (!try_times) {
		printk_err("step2: recv temp and hum datas error in polling mode.\n");
			
		return -1;
	}
	
	ret = gxht3x_chk_crc(te_hu_data, 2, te_hu_data[2]);
	if (ret) {
		printk_err("temp data crc check error in polling mode.\n");
		
		return -1;
	}
	ret = gxht3x_chk_crc(&te_hu_data[3], 2, te_hu_data[5]);
	if (ret) {
		printk_err("humi data crc check error in polling mode.\n");
		
		return -1;
	}
	
	*temp = gxht3x_calc_temp((te_hu_data[0] << 8) | te_hu_data[1]);
	*humi = gxht3x_calc_humi((te_hu_data[3] << 8) | te_hu_data[4]);
	
	return 0;
}

static int read_temp_humi_data_gxht3x(int *temp, uint32_t *humi, gxht3x_attri_s *gxht3x)
{
	int ret = 0;
	
	switch (gxht3x->mode) {    
		case MODE_CLKSTRETCH: 	// get temperature with clock stretching mode
			ret = gxht3x_get_temp_humi_clkstretch(gxht3x, temp, humi);
			break;
		case MODE_POLLING:    	// get temperature with polling mode
			ret = gxht3x_get_temp_humi_polling(gxht3x, temp, humi);
			break;
		default:              
			ret = -1;
			break;
  }
  
  return ret;
}

static int gxht3x_init(gxht3x_attri_s *gxht3x)
{
	int ret = 0;
	
	if (I2C_EMULATE == gxht3x->oper_mode) {
		gxht3x->write_data	= i2c_emulate_write_data;
		gxht3x->read_data 	= i2c_emulate_read_data;
	} else {
		gxht3x->write_data	= i2c_module_write_data;
	}   gxht3x->read_data 	= i2c_module_read_data;

	if (gxht3x->client) {
		gxht3x->handle = (void *)gxht3x->client;
	} else {
		gxht3x->handle = (void *)gxht3x->emu_client;
	}

	ret = gxht3x_recv_serial_data(gxht3x);
	if (ret) {
		printk_err("get serial number error.\n");
		
		return -1;
	}

	return 0;
}

int gxht3x_open(void *chip)
{
	int ret = 0;
	gxht3x_attri_s *gxht3x_one = (gxht3x_attri_s *)chip;
	
	ret = gxht3x_init(gxht3x_one);
	if (ret < 0) {
		printk_err("open gxht3x error.\n");
		
		return -1;
	}
	
    return 0;
}

int gxht3x_close(void *chip)
{
	int ret = 0;
//	gxht3x_attri_s *gxht3x_one = (gxht3x_attri_s *)chip;

	
    return ret;
}

int gxht3x_read(void *chip, char *buf, size_t count)
{
	int ret			= 0;
	int temp 		= 0;
	uint32_t humi 	= 0;
	gxht3x_attri_s *gxht3x_one = (gxht3x_attri_s *)chip;
 
	(void)read_temp_humi_data_gxht3x(&temp, &humi, gxht3x_one);
	if (count != 8) {
		printk_err("the length of reading data error.\n");
		return -1;
	}

	buf[0] = (uint8_t)(temp);
	buf[1] = (uint8_t)(temp >> 8);
	buf[2] = (uint8_t)(temp >> 16);
	buf[3] = (uint8_t)(temp >> 24);
	buf[4] = (uint8_t)(humi);
	buf[5] = (uint8_t)(humi >> 8);
	buf[6] = (uint8_t)(humi >> 16);
	buf[7] = (uint8_t)(humi >> 24);	
	
	return ret;
}
