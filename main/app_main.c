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
#include "flash.h"

#define COMMAND_A "A"
#define COMMAND_B "B"
#define COMMAND_C "C"
QueueHandle_t uart0_queue;
char *TAG = "uart_events";

static uint8_t flash_data;
static uint8_t flag_delay = TIME_DELAY_1S;
static uint8_t flag_run = SEND_UP_DATA_STATUS;
static TaskHandle_t xHandle;

static esp_err_t err_flash;
static nvs_handle_t my_handle_flash;

int temperature;
int humidity;

void uart_up_data_host_main(void *pvParameters);
void input_event_callback(int pin);
void read_data_DHT11(void *pvParameters);
void uart_up_data_host_main(void *pvParameters);
void uart_receive_data_host_main(void *pvParameters);

void app_main(void)
{
    flash_init(&err_flash);
    // #if (0)

    input_io_create(BUTTON_1, HIGH_TO_LOW);
    input_io_create(BUTTON_2, HIGH_TO_LOW);
    input_set_callback(input_event_callback);

    output_io_create(BLINK_GPIO);
    output_io_create(LED_BLUE);
    output_io_create(LED_GREEN);
    output_io_create(LED_RED);

    DHT11_init(DHT_DATA);

    //    Create a task to handler UART event from ISR
    printf("Init\n");

    flash_open(&err_flash, &my_handle_flash);
    flag_run = flash_read_u8(&err_flash, &my_handle_flash, &flash_data);
    flag_run /= 10;
    printf("flash_data: %d\n", flash_data);
    printf("flag_run: %d\n", flag_run);
    nvs_close(&my_handle_flash);

    xTaskCreate(read_data_DHT11, "read_data_DHT11", 2048, NULL, 5, NULL);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uart_create();
    xTaskCreate(uart_up_data_host_main, "uart_up_data_host_main", 2048, NULL, 4, &xHandle);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uart_create();
    xTaskCreate(uart_receive_data_host_main, "uart_receive_data_host_main", 2048, NULL, 3, NULL);

// #endif
#if (0)
    flash_open(&err_flash, &my_handle_flash);
    while (err_flash != ESP_OK)
        flash_open(&err_flash, &my_handle_flash);
    flash_data = 0;
    flash_write_u8(&err_flash, &my_handle_flash, &flash_data);

#endif
}

void input_event_callback(int pin)
{
    if (pin == BUTTON_1)
    {
        flash_open(&err_flash, &my_handle_flash);
        while (err_flash != ESP_OK)
            flash_open(&err_flash, &my_handle_flash);
        uint8_t flag = flash_read_u8(&err_flash, &my_handle_flash, &flash_data);
        nvs_close(&my_handle_flash);
        flag /= 10;
        if (flag == SEND_UP_DATA_STATUS)
        {
            vTaskSuspend(xHandle);
        }
        else if (flag == PAUSE_UP_DATA_STATUS)
        {
            vTaskResume(xHandle);
        }

        flash_open(&err_flash, &my_handle_flash);
        while (err_flash != ESP_OK)
            flash_open(&err_flash, &my_handle_flash);
        flag_run = 1 - flag_run;
        flash_data = flash_data % 10;
        flash_data += 10 * flag_run;
        flash_write_u8(&err_flash, &my_handle_flash, &flash_data);
    }
    else if (pin == BUTTON_2)
    {
        output_io_toggle(BLINK_GPIO);
        flash_open(&err_flash, &my_handle_flash);
        while ((err_flash != ESP_OK))
            flash_open(&err_flash, &my_handle_flash);
        flag_delay = flash_data % 10;
        flag_delay = (flag_delay + 1) % 4;
        flash_data = flash_data / 10;
        flash_data = flash_data * 10 + flag_delay;
        flash_write_u8(&err_flash, &my_handle_flash, &flash_data);
    }
}

void read_data_DHT11(void *pvParameters)
{

    // printf("run in read_data_DHT11\n");
    for (;;)
    {
        if (DHT11_read().status == DHT11_OK)
        {
            temperature = DHT11_read().temperature;
            humidity = DHT11_read().humidity;

            if ((temperature < LEVEL_TEMPERATURE) && (humidity < LEVEL_HUMIDITY))
            {
                output_io_set_level(LED_GREEN, LOW_LEVEL);
                output_io_set_level(LED_BLUE, HIGH_LEVEL);
            }
            else if ((temperature < LEVEL_TEMPERATURE) && (humidity >= LEVEL_HUMIDITY))
            {
                output_io_set_level(LED_BLUE, HIGH_LEVEL);
                output_io_set_level(LED_GREEN, HIGH_LEVEL);
            }
            else if ((temperature >= LEVEL_TEMPERATURE) && (humidity < LEVEL_HUMIDITY))
            {
                output_io_set_level(LED_BLUE, LOW_LEVEL);
                output_io_set_level(LED_GREEN, HIGH_LEVEL);
            }
            else
            {
                output_io_set_level(LED_RED, HIGH_LEVEL);
            }
        }
        vTaskDelay(FREQUENCY_1S / portTICK_PERIOD_MS);
    }
}

void uart_up_data_host_main(void *pvParameters)
{
    // printf("run in uart_up_data_host_main\n");
    char chuoi_temp[13] = {'N', 'h', 'i', 'e', 't', ' ', 'd', 'o', ':', ' '};
    chuoi_temp[12] = '\n';
    char chuoi_hum[11] = {'D', 'o', ' ', 'a', 'm', ':', ' '};
    chuoi_hum[10] = '\n';
    for (;;)
    {
        if (flag_run == 0)
        {
            printf("\n");
            convertString(temperature, chuoi_temp + 10);
            convertString(humidity, chuoi_hum + 7);

            uart_write_bytes(EX_UART_NUM, chuoi_temp, 13);
            uart_write_bytes(EX_UART_NUM, chuoi_hum, 11);
        }

        if (flag_delay == TIME_DELAY_1S)
        {
            vTaskDelay(FREQUENCY_1S / portTICK_PERIOD_MS);
        }
        else if (flag_delay == TIME_DELAY_5S)
        {
            vTaskDelay(FREQUENCY_5S / portTICK_PERIOD_MS);
        }
        else if (flag_delay == TIME_DELAY_10S)
        {
            vTaskDelay(FREQUENCY_10S / portTICK_PERIOD_MS);
        }
        else
        {
            vTaskDelay(FREQUENCY_15S / portTICK_PERIOD_MS);
        }
    }
}

void uart_receive_data_host_main(void *pvParameters)
{
    // printf("run in uart_receive_data_host_main\n");
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE);
    // printf("run in uart_receive_data_host_main\n");
    for (;;)
    {
        // Waiting for UART event.
        // printf("run in UART event\n");
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            printf("run in xQueueReceive\n");
            bzero(dtmp, RD_BUF_SIZE);
            // ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch (event.type)
            {
            // Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                // ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                if (strcmp((char *)dtmp, COMMAND_A) == 0)
                {
                    flash_open(&err_flash, &my_handle_flash);
                    flash_data = flash_data % 10 + 10;
                    flag_run = 1;
                    flash_write_u8(&err_flash, &my_handle_flash, &flash_data);
                    vTaskSuspend(xHandle);
                }
                else if (strcmp((char *)dtmp, COMMAND_B) == 0)
                {
                    flash_open(&err_flash, &my_handle_flash);
                    flash_data = flash_data % 10;
                    flag_run = 0;
                    flash_write_u8(&err_flash, &my_handle_flash, &flash_data);
                    vTaskResume(xHandle);
                }
                else if (strcmp((char *)dtmp, COMMAND_C) == 0)
                {
                    flash_open(&err_flash, &my_handle_flash);
                    flag_delay = flash_data % 10;
                    flag_delay = (flag_delay + 1) % 4;
                    flash_data /= 10;
                    flash_data = flash_data * 10 + flag_delay;
                    flash_write_u8(&err_flash, &my_handle_flash, &flash_data);
                }
                break;
            // Others
            default:
                // ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}