#
# Makefile for kernel the whole drivers.
#
PRJ_PATH    	:= $(shell pwd)
KSRC 			:= $(PRJ_PATH)/../kernel-4.4.94/
ARCH 			:= mips
CROSS_COMPILE   := mips-linux-gnu-
MODULE_NAME 	:= driver_main

EXTRA_CFLAGS += -I$(PWD)/include/kernel -I$(PWD)/include/usr
EXTRA_LDFLAGS += --strip-debug
EXTRA_CFLAGS += -Werror

CONFIG_PG_CONFIG_EDR = y
CONFIG_GPIO_MODULE = n
CONFIG_I2C_MODULE = y
CONFIG_I2C_EMULATE = y
CONFIG_SPI_MODULE = y
CONFIG_SPI_EMULATE = y
CONFIG_MOTOR_TMI8150 = y
CONFIG_MOTOR_MODULE = y
CONFIG_TE_HU_GXHT3X = y
CONFIG_TE_HU_MODULE = y
CONFIG_ACC_GYRO_QMI8658 = y
CONFIG_ACC_GYRO_MODULE = y
CONFIG_FUEL_GAUGE_CW221X = y
CONFIG_FUEL_GAUGE_MODULE = y

obj-m := $(MODULE_NAME).o

$(MODULE_NAME)-$(CONFIG_GPIO_MODULE) += src/common/pg_gpio.o
$(MODULE_NAME)-$(CONFIG_I2C_MODULE) += src/driver/i2c/i2c_module/i2c_module.o
$(MODULE_NAME)-$(CONFIG_I2C_EMULATE) += src/driver/i2c/i2c_emulate/i2c_emulate.o
$(MODULE_NAME)-$(CONFIG_SPI_MODULE) += src/driver/spi/spi_module/spi_module.o
$(MODULE_NAME)-$(CONFIG_SPI_EMULATE) += src/driver/spi/spi_emulate/spi_emulate.o

$(MODULE_NAME)-$(CONFIG_MOTOR_TMI8150) += src/motor/motor.o
$(MODULE_NAME)-$(CONFIG_MOTOR_MODULE) += src/motor/tmi8150b.o
$(MODULE_NAME)-$(CONFIG_TE_HU_GXHT3X) += src/te_hu/gxht3x.o
$(MODULE_NAME)-$(CONFIG_TE_HU_MODULE) += src/te_hu/te_hu.o
$(MODULE_NAME)-$(CONFIG_ACC_GYRO_QMI8658) += src/acc_gyro/qmi8658a.o
$(MODULE_NAME)-$(CONFIG_ACC_GYRO_MODULE) += src/acc_gyro/acc_gyro.o
$(MODULE_NAME)-$(CONFIG_FUEL_GAUGE_CW221X) += src/fuel_gauge/cw2218.o
$(MODULE_NAME)-$(CONFIG_FUEL_GAUGE_MODULE) += src/fuel_gauge/fuel_gauge.o

$(MODULE_NAME)-$(CONFIG_PG_CONFIG_EDR) += src/config/pg_edr.o

$(MODULE_NAME)-objs += pg_driver.o

.PHONY: all clean

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PRJ_PATH) modules

clean:
	@find \( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
	-o -name '*.ko.*' -o -name '*.symvers' \
	-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
	-o -name '*.symtypes' -o -name 'modules.order' \
	-o -name modules.builtin -o -name '.tmp_*.o.*' \
	-o -name '*.gcno' \) -type f -print | xargs rm -f
