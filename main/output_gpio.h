/**
 * @file output_gpio.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for output with GPIO, API "create", "set level", "toggle" for others functions
 */
#ifndef OUTPUT_GPIO_H
#define OUTPUT_GPIO_H
#define BLINK_GPIO GPIO_NUM_2
#define LED_GREEN GPIO_NUM_26
#define LED_BLUE GPIO_NUM_25
#define LED_RED GPIO_NUM_27
#define HIGH_LEVEL 1
#define LOW_LEVEL 0
#include "esp_err.h"
#include "hal/gpio_types.h"

void output_vCreate(gpio_num_t gpio_num);
void output_vSetLevel(gpio_num_t gpio_num, int level);
void output_vToggle(gpio_num_t gpio_num);
#endif