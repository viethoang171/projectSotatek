#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"
#include <driver/uart.h>
#include "string.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "input_iot.h"
#include "output_iot.h"
#include "uart.h"
#include "DHT.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "flash.h"

uint8_t flag_delay;
uint8_t flag_run;
TaskHandle_t xHandle;

static esp_err_t err_run;
static nvs_handle_t my_handle_run;
static esp_err_t err_delay;
static nvs_handle_t my_handle_delay;

void uart_up_data_host_main(void *pvParameters);
void input_event_callback(int pin)
{
    if (pin == BUTTON_1)
    {
        flash_open(&err_run, &my_handle_run);
        //  if (flash_read_u8(&err_run, &my_handle_run, &flag_run) == 1)
        //      output_io_set_level(BLINK_GPIO, 1);
        if (flash_read_u8(&err_run, &my_handle_run, &flag_run) == 0)
        {
            output_io_set_level(BLINK_GPIO, 1);
            vTaskSuspend(xHandle);
        }
        else if (flash_read_u8(&err_run, &my_handle_run, &flag_run) == 1)
            vTaskResume(xHandle);
        nvs_close(&my_handle_run);
        flag_run = 1 - flag_run;
        flash_open(&err_run, &my_handle_run);
        flash_write_u8(&err_run, &my_handle_run, &flag_run);
        // if (flash_write_u8(&err_run, &my_handle_run, &flag_run) == 1)
        //     output_io_set_level(LED_GREEN, 1);
    }
    else if (pin == BUTTON_2)
    {
        // flash_open(&err_delay, &my_handle_delay);
        flag_delay = (flag_delay + 1) % 4;
        // flash_write_u8(&err_delay, &err_delay, &flag_delay);
    }
}

void uart_up_data_host_main(void *pvParameters)
{
    printf("run in uart_up_data_host_main\n");
    flash_open(&err_run, &my_handle_run);
    if (flash_read_u8(&err_run, &my_handle_run, &flag_run) == 1)
    {
        nvs_close(&my_handle_run);
        vTaskSuspend(NULL);
    }

    char chuoi_temp[14] = {'N', 'h', 'i', 'e', 't', ' ', 'd', 'o', ':', ' '};
    chuoi_temp[13] = '\n';
    char chuoi_hum[11] = {'D', 'o', ' ', 'a', 'm', ':', ' '};
    chuoi_hum[10] = '\n';
    for (;;)
    {
        flash_open(&err_run, &my_handle_run);
        if (flash_read_u8(&err_run, &my_handle_run, &flag_run) == 1)
        {
            nvs_close(&my_handle_run);
            vTaskSuspend(NULL);
        }
        convertString(DHT11_read().temperature, chuoi_temp + 10);
        convertString(DHT11_read().humidity, chuoi_hum + 7);
        // if (DHT11_read().temperature < 27 && DHT11_read().humidity < 80)
        // {
        //     output_io_set_level(LED_GREEN, 0);
        //     output_io_set_level(LED_BLUE, 1);
        // }
        // else if (DHT11_read().temperature < 27 && DHT11_read().humidity >= 80)
        // {
        //     output_io_set_level(LED_BLUE, 1);
        //     output_io_set_level(LED_GREEN, 1);
        // }
        // else if (DHT11_read().temperature >= 27 && DHT11_read().humidity < 80)
        // {
        //     output_io_set_level(LED_BLUE, 0);
        //     output_io_set_level(LED_GREEN, 1);
        // }
        // else
        // {
        //     output_io_set_level(LED_RED, 1);
        // }

        uart_write_bytes(0, chuoi_temp, 14);
        uart_write_bytes(0, chuoi_hum, 11);
        if (flag_delay == 0)
            vTaskDelay(FREQUENCY_1S / portTICK_PERIOD_MS);
        else if (flag_delay == 1)
            vTaskDelay(FREQUENCY_5S / portTICK_PERIOD_MS);
        else if (flag_delay == 2)
            vTaskDelay(FREQUENCY_10S / portTICK_PERIOD_MS);
        else
            vTaskDelay(FREQUENCY_15S / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    printf("Init\n");
    flash_init(&err_run);
    flash_open(&err_run, &my_handle_run);
    if (err_run == ESP_OK)
    {
        flag_run = 0;
        flash_write_u8(&err_run, &my_handle_run, &flag_run);
    }
    // flash_open(&err_run, &my_handle_run);
    // if (err_run == ESP_OK)
    // {
    //     flash_write_u8(&err_run, &my_handle_run, &flag_delay);
    //     flash_open(&err_run, &my_handle_run);
    //     flash_read_u8(&err_run, &my_handle_run, &flag_run);
    // }
    flash_init(&err_delay);

    input_io_create(BUTTON_1, HIGH_TO_LOW);
    input_io_create(BUTTON_2, HIGH_TO_LOW);
    input_set_callback(input_event_callback);

    output_io_create(BLINK_GPIO);
    output_io_create(LED_BLUE);
    output_io_create(LED_GREEN);
    output_io_create(LED_RED);

    DHT11_init(GPIO_NUM_4);

    // uart_create();
    // xTaskCreate(uart_receive_data_host_main, "uart_receive_data_host_main", 2048, NULL, 3, NULL);

    //    Create a task to handler UART event from ISR
    uart_create();
    xTaskCreate(uart_up_data_host_main, "uart_up_data_host_main", 2048, NULL, 4, &xHandle);

    uart_create();
    xTaskCreate(uart_receive_data_host_main, "uart_receive_data_host_main", 2048, NULL, 3, NULL);
}
