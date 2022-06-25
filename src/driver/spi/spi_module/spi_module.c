#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <config/pg_spi.h>

int spi_module_write_data(void *handle, const reg_t *s_reg)
{
    int ret = 0;
	struct spi_transfer *t;
    struct spi_message m;
    struct spi_device *spi = (struct spi_device *)handle;
 
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    if(t == NULL) {
        printk_err("kzalloc spi_transfer error!\r\n");
        return -1;
    }
    memset(t, 0x00, sizeof(struct spi_transfer));
    memset(&m, 0x00, sizeof(struct spi_message));

    t->tx_buf 	= s_reg->buf;
    t->len 		= s_reg->len;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);
    if(ret < 0) {
        printk_err("spi_sync failed!\r\n");
        kfree(t);
        return -1;
    }
 
    kfree(t);

    return 0;
}

int spi_module_read_data(void *handle, const reg_t *s_reg, const buf_t *r_buf)
{
	int ret = 0;
    struct spi_device *spi = (struct spi_device *)handle;
    
    ret = spi_write_then_read(spi, s_reg->buf, s_reg->len, r_buf->buf, r_buf->len);
    if (ret < 0) {
        printk_err("spi write then read error.\n");
        return -1;
    }

	return 0;
}
