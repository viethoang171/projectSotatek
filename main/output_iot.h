#ifndef OUTPUT_IOT_H
#define OUTPUT_IOT_H
#define BLINK_GPIO GPIO_NUM_2
#define LED_GREEN GPIO_NUM_26
#define LED_BLUE GPIO_NUM_25
#define LED_RED GPIO_NUM_27
#define HIGH_LEVEL 1
#define LOW_LEVEL 0
#include "esp_err.h"
#include "hal/gpio_types.h"

void output_io_create(gpio_num_t gpio_num);
void output_io_set_level(gpio_num_t gpio_num, int level);
void output_io_toggle(gpio_num_t gpio_num);
#endif