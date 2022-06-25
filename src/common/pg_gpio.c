#include <config/pg_gpio.h>

int pg_used_gpio_init(pg_used_gpio_t *used_pins, uint8_t pin_num)
{
	uint8_t i 	= 0;
    int 	ret	= 0;

	for (i = 0; i < pin_num; i++) {
		if (-1 == used_pins->gpio){		//不使用
			used_pins++;
			continue;
		}
		ret |= gpio_request(used_pins->gpio, used_pins->pin_name);

		if (INPUT_PIN == used_pins->direction) {		//input
			gpio_direction_input(used_pins->gpio);
		}else {
			gpio_direction_output(used_pins->gpio, used_pins->init_val);
		}
		used_pins++;
	}

    return ret;	
}

int pg_used_gpio_deinit(pg_used_gpio_t *used_pins, uint8_t pin_num)
{
	uint8_t i = 0;

	for (i = 0; i < pin_num; i++) {
		if (-1 == used_pins->gpio){		//不使用
			used_pins++;
			continue;
		}		
		gpio_free(used_pins->gpio);
		used_pins++;
	}

    return 0;
}
