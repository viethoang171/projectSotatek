/**
 * @file bee_Led.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for output with GPIO, API "create", "set level", "toggle" for others functions
 */
#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "bee_Led.h"

void output_vCreate(gpio_num_t gpio_num)
{
    esp_rom_gpio_pad_select_gpio(gpio_num);
    // Set the GPIO as a push/pull output
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT_OUTPUT);
}

void output_vSetLevel(gpio_num_t gpio_num, int level)
{
    gpio_set_level(gpio_num, level);
}

void output_vToggle(gpio_num_t gpio_num)
{
    int previous_level = gpio_get_level(gpio_num);
    gpio_set_level(gpio_num, 1 - previous_level);
}

void output_vSetWarningLed(uint8_t u8Temperature, uint8_t u8Humidity)
{
    if ((u8Temperature < LEVEL_TEMPERATURE) && (u8Humidity < LEVEL_HUMIDITY))
    {
        output_vSetLevel(LED_GREEN, LOW_LEVEL);
        output_vSetLevel(LED_BLUE, HIGH_LEVEL);
        output_vSetLevel(LED_RED, LOW_LEVEL);
    }
    else if ((u8Temperature < LEVEL_TEMPERATURE) && (u8Humidity >= LEVEL_HUMIDITY))
    {
        output_vSetLevel(LED_BLUE, HIGH_LEVEL);
        output_vSetLevel(LED_GREEN, HIGH_LEVEL);
        output_vSetLevel(LED_RED, LOW_LEVEL);
    }
    else if ((u8Temperature >= LEVEL_TEMPERATURE) && (u8Humidity < LEVEL_HUMIDITY))
    {
        output_vSetLevel(LED_BLUE, LOW_LEVEL);
        output_vSetLevel(LED_GREEN, HIGH_LEVEL);
        output_vSetLevel(LED_RED, LOW_LEVEL);
    }
    else
    {
        output_vSetLevel(LED_BLUE, LOW_LEVEL);
        output_vSetLevel(LED_GREEN, LOW_LEVEL);
        output_vSetLevel(LED_RED, HIGH_LEVEL);
    }
}
void output_vSetWarning_not_read()
{
    output_vSetLevel(LED_GREEN, LOW_LEVEL);
    output_vSetLevel(LED_BLUE, LOW_LEVEL);
    output_vSetLevel(LED_RED, LOW_LEVEL);
}