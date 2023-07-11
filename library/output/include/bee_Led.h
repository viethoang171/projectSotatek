/**
 * @file bee_Led.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for output with GPIO, API "create", "set level", "toggle" for others functions
 */
#ifndef BEE_LED_H
#define BEE_LED_H

#define BLINK_GPIO GPIO_NUM_2
#define LED_GREEN GPIO_NUM_26
#define LED_BLUE GPIO_NUM_25
#define LED_RED GPIO_NUM_27

#define HIGH_LEVEL 1
#define LOW_LEVEL 0

#define LEVEL_TEMPERATURE 27
#define LEVEL_HUMIDITY 80

#include "esp_err.h"
#include "hal/gpio_types.h"

void output_vCreate(gpio_num_t gpio_num);
void output_vSetLevel(gpio_num_t gpio_num, int level);
void output_vToggle(gpio_num_t gpio_num);
void output_vSetWarningLed(uint8_t u8Temperature, uint8_t u8Humidity);
#endif