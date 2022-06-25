#ifndef __PG_GPIO_H__
#define __PG_GPIO_H__

#include <linux/gpio.h>
#include "pg_typedefs.h"

#define OUTPUT_PIN			0
#define INPUT_PIN			1

#define LOW_LEVEL			0
#define HIGH_LEVEL			1

typedef struct pg_used_gpio {
	unsigned 	gpio;	
	const char 	*pin_name;
	char		direction;
	int			init_val;
}pg_used_gpio_t;

int pg_used_gpio_init(pg_used_gpio_t *used_pins, uint8_t pin_num);
int pg_used_gpio_deinit(pg_used_gpio_t *used_pins, uint8_t pin_num);

#endif