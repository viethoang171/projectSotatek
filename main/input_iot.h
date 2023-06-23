#ifndef INPUT_IOT_H
#define INPUT_IOT_H
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

void input_io_create(gpio_num_t gpio_num, interrupt_type_edge_t type);
void input_set_callback(void *callbackk);
#endif