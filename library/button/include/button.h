/**
 * @file button.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for input button with GPIO, API "create", "call back" for others functions
 */
#ifndef BUTTON_H
#define BUTTON_H
#define BUTTON_2 GPIO_NUM_0
#define BUTTON_1 GPIO_NUM_13
#include "esp_err.h"
#include "hal/gpio_types.h"

typedef enum
{
    LOW_TO_HIGH = 1,
    HIGH_TO_LOW,
    ANY_EDGE
} interrupt_type_edge_t;
typedef enum
{
    FREQUENCY_1S = 1000,
    FREQUENCY_5S = 5000,
    FREQUENCY_10S = 10000,
    FREQUENCY_15S = 15000,
} frequency;

typedef void (*input_callback_t)(int);

void button_vCreateInput(gpio_num_t gpio_num, interrupt_type_edge_t type);
void button_vSetCallback(void *callbackk);
#endif