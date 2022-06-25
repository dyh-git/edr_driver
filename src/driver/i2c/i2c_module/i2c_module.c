#include <linux/i2c.h>
#include <config/pg_i2c.h>

int i2c_module_write_data(void *handle, const reg_t *s_reg)
{
	int ret = 0;
	struct i2c_client *client = (struct i2c_client *)handle;
	struct i2c_msg msg;

	memset(&msg, 0x00, sizeof(struct i2c_msg));
	msg.addr 	= client->addr;
	msg.flags 	= 0;
	msg.len   	= s_reg->len;
	msg.buf 	= s_reg->buf;
	
	ret = i2c_transfer(client->adapter, &msg, 1);
	if(ret <= 0)
	{
		printk_err("transfer cmd data error.\n");
	}

	return ret;
}

int i2c_module_read_data(void *handle, const reg_t *s_reg, const buf_t *r_buf)
{
	int ret = 0;
	struct i2c_client *client = (struct i2c_client *)handle;
    struct i2c_msg msg[] = {
		{
			.addr  = client->addr,
			.flags = 0,
			.len   = s_reg->len,     		//表示寄存器地址字节长度，以byte为单位
			.buf   = s_reg->buf,
		},
		{
			.addr  = client->addr,
			.flags = I2C_M_RD,
			.len   = r_buf->len, 			//表示期望读到数据的字节长度，以byte为单位
			.buf   = r_buf->buf, 			//将读取到的数据保存在buffer中
		},
	};
	
	ret = i2c_transfer(client->adapter, msg, sizeof(msg)/sizeof(msg[0]));
    if(ret != (sizeof(msg)/sizeof(msg[0]))) {
		printk_err("read regs from chip error.\n");
		
		return -1;
	}
    
    return 0;
}
