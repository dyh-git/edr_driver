#include <config/pg_typedefs.h>
#include <acc_gyro/qmi8658a.h>

static qmi8658_state g_imu;

static int qmi8658_send_data(qmi8658_attri_s *qmi8658, const reg_t *s_reg)
{
	int ret = 0;

	if (!(qmi8658->write_data)) {
		printk_err("write_data function error.\n");
		
		return -1;
	}
	
	ret = qmi8658->write_data(qmi8658->handle, s_reg);
	if (ret < 0)
		printk_err("write data error.\n");
	
	return ret;	
}

static int qmi8658_rcv_data(qmi8658_attri_s *qmi8658, const reg_t *s_reg, const buf_t *r_buf)
{
	int ret = 0;

	if (!(qmi8658->read_data)) {
		printk_err("read_data function error.\n");
		
		return -1;
	}
		
	ret = qmi8658->read_data(qmi8658->handle, s_reg, r_buf);
	if (ret < 0)
		printk_err("read data error.\n");
	
	return ret;	
}

static int qmi8658_write_reg(qmi8658_attri_s *qmi8658, uint8_t reg_addr, uint8_t data)
{
	int ret 			= 0;
	uint8_t msg_data[2] = {0x00};
	reg_t s_reg;
	
	msg_data[0] = reg_addr;
	msg_data[1] = data;
	s_reg.buf = msg_data;
	s_reg.len = 2;
	
	ret = qmi8658_send_data(qmi8658, &s_reg);
	if (ret < 0) {
		printk_err("qmi8658 read data error.\n");
		return -1;
	}

	return ret;
}

static int qmi8658_read_reg(qmi8658_attri_s *qmi8658, uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
	int ret = 0;
	reg_t s_reg;
	buf_t r_buf;
	
	s_reg.buf = &reg_addr;
	s_reg.len = 1;
	r_buf.buf = buf;
	r_buf.len = len;
	
	ret = qmi8658_rcv_data(qmi8658, &s_reg, &r_buf);
	if (ret < 0) {
		printk_err("qmi8658 read data error.\n");
		return -1;
	}
	
	return ret;
}

static void qmi8658_delay(unsigned int ms)
{
	mdelay(ms);

	return;
}

static uint8_t qmi8658_get_id(qmi8658_attri_s *qmi8658)
{
	uint8_t retry 			= 0;
	uint8_t qmi8658_chip_id = 0x00;
	uint8_t qmi8658_revision_id = 0x00;
	uint8_t firmware_id[3]		= {0x00};
	uint8_t uuid[6]		= {0x00};
	uint32_t uuid_low	= 0x00;
	uint32_t uuid_high	= 0x00;

	while((qmi8658_chip_id != 0x05) && (retry++ < 5)) {
		qmi8658_read_reg(qmi8658, Qmi8658Register_WhoAmI, &qmi8658_chip_id, 1);
		printk_log("error Qmi8658Register_WhoAmI = 0x%x\n", qmi8658_chip_id);
	}
	
	if(qmi8658_chip_id == 0x05)
	{
		printk_log("qmi8658_on_demand_cali start\n");
		qmi8658_write_reg(qmi8658, Qmi8658Register_Reset, 0xb0);
		qmi8658_delay(10);		// delay
		qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl9, qmi8658_Ctrl9_Cmd_On_Demand_Cali);
		qmi8658_delay(2200);	// delay 2000ms above
		qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl9, qmi8658_Ctrl9_Cmd_NOP);
		qmi8658_delay(100);		// delay
		printk_log("qmi8658_on_demand_cali done\n");
		
		g_imu.cfg.ctrl8_value = 0xc0;
		//QMI8658_INT1_ENABLE, QMI8658_INT2_ENABLE
		qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl1, 0x60 | QMI8658_INT2_ENABLE | QMI8658_INT1_ENABLE);
		qmi8658_read_reg(qmi8658, Qmi8658Register_Revision, &qmi8658_revision_id, 1);			
		qmi8658_read_reg(qmi8658, Qmi8658Register_firmware_id, firmware_id, 3);
		qmi8658_read_reg(qmi8658, Qmi8658Register_uuid, uuid, 6);
		qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl7, 0x00);
		qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl8, g_imu.cfg.ctrl8_value);
		uuid_low 	= (uint32_t)((uint32_t)(uuid[2] << 16) | (uint32_t)(uuid[1] << 8) | (uuid[0]));
		uuid_high 	= (uint32_t)((uint32_t)(uuid[5] << 16) | (uint32_t)(uuid[4] << 8) | (uuid[3]));
		
		printk_log("qmi8658_revision_id=0x%x\n", qmi8658_revision_id);
		printk_log("Firmware ID[0x%x 0x%x 0x%x]\n", firmware_id[2], firmware_id[1], firmware_id[0]);
		printk_log("UUID[0x%x %x]\n", uuid_high, uuid_low);

	}

	return qmi8658_chip_id;
}

static void qmi8658_enableSensors(qmi8658_attri_s *qmi8658, uint8_t en_flags)
{

	qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl7, en_flags);

	g_imu.cfg.enSensors = en_flags & 0x03;

	qmi8658_delay(1);
	
	return;
}

static void qmi8658_config_acc(qmi8658_attri_s *qmi8658, enum qmi8658_AccRange range, enum qmi8658_AccOdr odr, enum qmi8658_LpfConfig lpfEnable, enum qmi8658_StConfig stEnable)
{
	uint8_t ctl_dada = 0;

	switch(range)
	{
		case Qmi8658AccRange_2g:
			g_imu.ssvt_a = (1 << 14);
			break;
		case Qmi8658AccRange_4g:
			g_imu.ssvt_a = (1 << 13);
			break;
		case Qmi8658AccRange_8g:
			g_imu.ssvt_a = (1 << 12);
			break;
		case Qmi8658AccRange_16g:
			g_imu.ssvt_a = (1 << 11);
			break;
		default: 
			range = Qmi8658AccRange_8g;
			g_imu.ssvt_a = (1 << 12);
	}
	
	if(stEnable == Qmi8658St_Enable)
		ctl_dada = (uint8_t)range | (uint8_t)odr | 0x80;
	else
		ctl_dada = (uint8_t)range | (uint8_t)odr;
	//Enable Accelerometer Self-Test
	qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl2, ctl_dada);

	//set accelerometer LPF
	qmi8658_read_reg(qmi8658, Qmi8658Register_Ctrl5, &ctl_dada, 1);
	ctl_dada &= 0xf0;
	if(lpfEnable == Qmi8658Lpf_Enable)
	{
		ctl_dada |= A_LSP_MODE_3;
		ctl_dada |= 0x01;
	}
	else
	{
		ctl_dada &= ~0x01;
	}
	qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl5, ctl_dada);
	
	return;
}

static void qmi8658_config_gyro(qmi8658_attri_s *qmi8658, enum qmi8658_GyrRange range, enum qmi8658_GyrOdr odr, enum qmi8658_LpfConfig lpfEnable, enum qmi8658_StConfig stEnable)
{
	uint8_t ctl_dada = 0; 

	//Store the scale factor for use when processing raw data
	switch (range)
	{
		case Qmi8658GyrRange_16dps:
			g_imu.ssvt_g = 2048;
			break;			
		case Qmi8658GyrRange_32dps:
			g_imu.ssvt_g = 1024;
			break;
		case Qmi8658GyrRange_64dps:
			g_imu.ssvt_g = 512;
			break;
		case Qmi8658GyrRange_128dps:
			g_imu.ssvt_g = 256;
			break;
		case Qmi8658GyrRange_256dps:
			g_imu.ssvt_g = 128;
			break;
		case Qmi8658GyrRange_512dps:
			g_imu.ssvt_g = 64;
			break;
		case Qmi8658GyrRange_1024dps:
			g_imu.ssvt_g = 32;
			break;
		case Qmi8658GyrRange_2048dps:
			g_imu.ssvt_g = 16;
			break;
		default: 
			range = Qmi8658GyrRange_512dps;
			g_imu.ssvt_g = 64;
			break;
	}

	//Enable Gyro self-Test
	if(stEnable == Qmi8658St_Enable)
		ctl_dada = (uint8_t)range | (uint8_t)odr | 0x80;
	else
		ctl_dada = (uint8_t)range | (uint8_t)odr;
	qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl3, ctl_dada);

	//set Gyroscope LPF
	qmi8658_read_reg(qmi8658, Qmi8658Register_Ctrl5, &ctl_dada, 1);
	ctl_dada &= 0x0f;
	if(lpfEnable == Qmi8658Lpf_Enable)
	{
		ctl_dada |= G_LSP_MODE_3;
		ctl_dada |= 0x10;
	}
	else
	{
		ctl_dada &= ~0x10;
	}
	qmi8658_write_reg(qmi8658, Qmi8658Register_Ctrl5, ctl_dada);

	return;
}

static void qmi8658_config_reg(qmi8658_attri_s *qmi8658, uint8_t low_power)
{
	qmi8658_enableSensors(qmi8658, QMI8658_DISABLE_ALL);
	if(low_power)
	{
		g_imu.cfg.enSensors = QMI8658_ACC_ENABLE;
		g_imu.cfg.accRange 	= Qmi8658AccRange_8g;
		g_imu.cfg.accOdr 	= Qmi8658AccOdr_LowPower_21Hz;
		g_imu.cfg.gyrRange 	= Qmi8658GyrRange_1024dps;
		g_imu.cfg.gyrOdr	= Qmi8658GyrOdr_250Hz;
	}
	else
	{		
		g_imu.cfg.enSensors = QMI8658_ACCGYR_ENABLE;
		g_imu.cfg.accRange 	= Qmi8658AccRange_8g;
		g_imu.cfg.accOdr 	= Qmi8658AccOdr_250Hz;
		g_imu.cfg.gyrRange 	= Qmi8658GyrRange_1024dps;
		g_imu.cfg.gyrOdr 	= Qmi8658GyrOdr_250Hz;
	}
	
	if(g_imu.cfg.enSensors & QMI8658_ACC_ENABLE)
	{
		qmi8658_config_acc(qmi8658, g_imu.cfg.accRange, g_imu.cfg.accOdr, Qmi8658Lpf_Disable, Qmi8658St_Disable);
	}
	if(g_imu.cfg.enSensors & QMI8658_GYR_ENABLE)
	{
		qmi8658_config_gyro(qmi8658, g_imu.cfg.gyrRange, g_imu.cfg.gyrOdr, Qmi8658Lpf_Disable, Qmi8658St_Disable);
	}
	
	return;
}

static void qmi8658_dump_reg(qmi8658_attri_s *qmi8658)
{
	uint8_t read_data[8] = {0x00};

	qmi8658_read_reg(qmi8658, Qmi8658Register_Ctrl1, read_data, 8);
	printk_log("Ctrl1[0x%x]\nCtrl2[0x%x]\nCtrl3[0x%x]\nCtrl4[0x%x]\nCtrl5[0x%x]\nCtrl6[0x%x]\nCtrl7[0x%x]\nCtrl8[0x%x]\n",
					read_data[0], read_data[1], read_data[2], read_data[3], read_data[4], read_data[5], read_data[6], read_data[7]);
	
	return;
}

static int qmi8658_init(qmi8658_attri_s *qmi8658)
{
	if(qmi8658_get_id(qmi8658) == 0x05)
	{
#if defined(QMI8658_USE_AMD)
		qmi8658_config_amd(qmi8658);
#endif
#if defined(QMI8658_USE_PEDOMETER)
		qmi8658_config_pedometer(qmi8658, 125);
		qmi8658_enable_pedometer(qmi8658, 1);
#endif
		qmi8658_config_reg(qmi8658, 0);
		qmi8658_enableSensors(qmi8658, g_imu.cfg.enSensors);
		qmi8658_dump_reg(qmi8658);
#if defined(QMI8658_USE_CALI)
		memset(&g_cali, 0, sizeof(g_cali));
#endif
		return 0;
	}

	printk_log("qmi8658_init fail\n");
	return -1;
}

static void qmi8658_axis_convert(qmi8658_attri_s *qmi8658, int *data_a, int *data_g, int layout)
{
	int raw_a[3] 	= {0x00};
	int raw_g[3] 	= {0x00};

	raw_a[0] = data_a[0];
	raw_a[1] = data_a[1];
	raw_g[0] = data_g[0];
	raw_g[1] = data_g[1];

	if((layout >= 4) && (layout <= 7)) {
		data_a[2] = -data_a[2];
		data_g[2] = -data_g[2];
	}

	if(layout % 2) {
		data_a[0] = raw_a[1];
		data_a[1] = raw_a[0];
		
		data_g[0] = raw_g[1];
		data_g[1] = raw_g[0];
	} else {
		data_a[0] = raw_a[0];
		data_a[1] = raw_a[1];

		data_g[0] = raw_g[0];
		data_g[1] = raw_g[1];
	}

	if((layout == 1) || (layout == 2) || (layout == 4) || (layout == 7)) {
		data_a[0] = -data_a[0];
		data_g[0] = -data_g[0];
	}
	
	if((layout == 2) || (layout == 3) || (layout == 6) || (layout == 7)) {
		data_a[1] = -data_a[1];
		data_g[1] = -data_g[1];
	}
	
	return;
}

static int qmi8658_readTemp(qmi8658_attri_s *qmi8658)
{
	uint8_t buf[2] 	= {0x00};
	int temp 		= 0;

	qmi8658_read_reg(qmi8658, Qmi8658Register_Tempearture_L, buf, 2);
	temp = ((int)buf[1] << 8)| buf[0];

	return temp;
}

static uint32_t qmi8658_read_timestamp(qmi8658_attri_s *qmi8658)
{
	uint8_t	buf[3]		= {0x00};
	uint32_t timestamp	= {0x00};

	qmi8658_read_reg(qmi8658, Qmi8658Register_Timestamp_L, buf, 3);
	timestamp = (uint32_t)(((uint32_t)buf[2] << 16) | ((uint32_t)buf[1] << 8) | buf[0]);
	if(timestamp > g_imu.timestamp) {
		g_imu.timestamp = timestamp;
	} else {
		g_imu.timestamp = (timestamp + 0x1000000 - g_imu.timestamp);
	}
	
	return g_imu.timestamp;
}

void qmi8658_read_sensor_data(qmi8658_attri_s *qmi8658, int acc[3], int gyro[3])
{
	uint8_t	buf_reg[12] = {0x00};
	int raw_acc_xyz[3]	= {0x00};
	int raw_gyro_xyz[3]	= {0x00};

	qmi8658_read_reg(qmi8658, Qmi8658Register_Ax_L, buf_reg, 12);
	raw_acc_xyz[0] = (int)((int)(buf_reg[1] << 8) | ( buf_reg[0]));
	raw_acc_xyz[1] = (int)((int)(buf_reg[3] << 8) | ( buf_reg[2]));
	raw_acc_xyz[2] = (int)((int)(buf_reg[5] << 8) | ( buf_reg[4]));

	raw_gyro_xyz[0] = (int)((int)(buf_reg[7] << 8) | ( buf_reg[6]));
	raw_gyro_xyz[1] = (int)((int)(buf_reg[9] << 8) | ( buf_reg[8]));
	raw_gyro_xyz[2] = (int)((int)(buf_reg[11] << 8) |( buf_reg[10]));

	if (qmi8658->mg_dps_mode) {
		// mg
		acc[0] = (int)((raw_acc_xyz[0] * 1000 * 100) / g_imu.ssvt_a);
		acc[1] = (int)((raw_acc_xyz[1] * 1000 * 100) / g_imu.ssvt_a);
		acc[2] = (int)((raw_acc_xyz[2] * 1000 * 100) / g_imu.ssvt_a);

		// dps
		gyro[0] = (int)((raw_gyro_xyz[0] * 1 * 100) / g_imu.ssvt_g);
		gyro[1] = (int)((raw_gyro_xyz[1] * 1 * 100) / g_imu.ssvt_g);
		gyro[2] = (int)((raw_gyro_xyz[2] * 1 * 100) / g_imu.ssvt_g);
	} else {
		// m/s2
		acc[0] = (int)((raw_acc_xyz[0] * ONE_G) / g_imu.ssvt_a);
		acc[1] = (int)((raw_acc_xyz[1] * ONE_G) / g_imu.ssvt_a);
		acc[2] = (int)((raw_acc_xyz[2] * ONE_G) / g_imu.ssvt_a);
		
		// rad/s
		gyro[0] = (int)((raw_gyro_xyz[0] * M_PI) / (g_imu.ssvt_g * 180));		// *pi/180
		gyro[1] = (int)((raw_gyro_xyz[1] * M_PI) / (g_imu.ssvt_g * 180));		// *pi/180
		gyro[2] = (int)((raw_gyro_xyz[2] * M_PI) / (g_imu.ssvt_g * 180));		// *pi/180
	}

	return;
}

static void qmi8658_read_xyz(qmi8658_attri_s *qmi8658, int *acc, int *gyro)
{
	uint8_t status = 0x00;

	qmi8658_read_reg(qmi8658, Qmi8658Register_Status0, &status, 1);
	if(status & 0x03)
	{
		qmi8658_read_sensor_data(qmi8658, acc, gyro);
		qmi8658_axis_convert(qmi8658, acc, gyro, 0);

		g_imu.imu[0] = acc[0];
		g_imu.imu[1] = acc[1];
		g_imu.imu[2] = acc[2];
		g_imu.imu[3] = gyro[0];
		g_imu.imu[4] = gyro[1];
		g_imu.imu[5] = gyro[2];
	}
	else
	{
		acc[0]  = g_imu.imu[0];
		acc[1]  = g_imu.imu[1];
		acc[2]  = g_imu.imu[2];
		gyro[0] = g_imu.imu[3];
		gyro[1] = g_imu.imu[4];
		gyro[2] = g_imu.imu[5];
		
		printk_log("data ready fail!\n");
	}
	
	return;
}

static void qmi8658_init_handle(qmi8658_attri_s *qmi8658)
{	
	if (I2C_EMULATE == qmi8658->oper_mode) {
		qmi8658->write_data = i2c_emulate_write_data;
		qmi8658->read_data 	= i2c_emulate_read_data;
	} else {
		qmi8658->write_data = i2c_module_write_data;
	}   qmi8658->read_data 	= i2c_module_read_data;
	
	if (qmi8658->client) {
		qmi8658->handle = (void *)qmi8658->client;
	} else {
		qmi8658->handle = (void *)qmi8658->emu_client;
	}

	return;
}

int qmi8658_open(void *chip)
{
	int ret = 0;
	
	qmi8658_attri_s *qmi8658_one = (qmi8658_attri_s *)chip;
	
	memset(&g_imu, 0x00, sizeof(qmi8658_state));
	qmi8658_init_handle(qmi8658_one);
	ret = qmi8658_init(qmi8658_one);
	if (ret < 0) {
		printk("qmi8658_init error.\n");
		
		return -1;
	}
	
	return 0;
}

int qmi8658_close(void *chip)
{
	int ret = 0;
//	qmi8658_attri_s *qmi8658_one = (qmi8658_attri_s *)chip;
	
	return ret;
}

int qmi8658_read(void *chip, char *buf, size_t count)
{
	uint8_t	data_type	= buf[0];
	int temp 			= 0;
	uint32_t timestamp 	= 0;
	int acc[3] 			= {0x00};
	int gyro[3] 		= {0x00};
	qmi8658_attri_s *qmi8658_one = (qmi8658_attri_s *)chip;
	
	switch (data_type) {
		case QMI8658_TEMP:
			temp = qmi8658_readTemp(qmi8658_one);
			buf[0] = (uint8_t)temp;
			buf[1] = (uint8_t)(temp >> 8);
			break;
		case QMI8658_TIMESTAMP:
			timestamp = qmi8658_read_timestamp(qmi8658_one);
			buf[0] = (uint8_t)timestamp;
			buf[1] = (uint8_t)(timestamp >> 8);
			buf[2] = (uint8_t)(timestamp >> 16);
			break;
		case QMI8658_XYZ:
			qmi8658_read_xyz(qmi8658_one, acc, gyro);
			memcpy(buf, acc, 12);
			memcpy((buf + 12), gyro, 12);
			break;
		default:
			printk_err("which data do you want.\n");
			return -1;
	}
	
	return 0;
}
