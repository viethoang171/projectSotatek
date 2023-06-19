#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "esp_attr.h"
#include "input_iot.h"

input_callback_t input_callback = NULL;

static void IRAM_ATTR gpio_input_handler(void *arg)
{
    int gpio_num = (uint32_t)arg;
    input_callback(gpio_num);
}

void input_io_create(gpio_num_t gpio_num, interrupt_type_edge_t type)
{
    esp_rom_gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(gpio_num, type);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio_num, gpio_input_handler, (void *)gpio_num);
}

void input_set_callback(void *callback)
{
    input_callback = callback;
}