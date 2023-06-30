/**
 * @file bee_App_main.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for project read signal DHT11, up data to host main, receive signal from host main
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"
#include <driver/uart.h>
#include "string.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "bee_Button.h"
#include "bee_Led.h"
#include "bee_Uart.h"
#include "bee_DHT.h"
#include "nvs_flash.h"
#include "bee_FLash.h"

QueueHandle_t uart0_queue;
char *TAG = "uart_events";

static uint8_t u8Flash_data;
static uint8_t u8Flag_delay = TIME_DELAY_1S;
static uint8_t u8Flag_run = SEND_UP_DATA_STATUS;
static TaskHandle_t xHandle;

static esp_err_t err_flash;
static nvs_handle_t my_handle_flash;

uint8_t u8Temperature;
uint8_t u8Humidity;

void button_vEventCallback(int pin);
void dht11_vReadDataDht11_task(void *pvParameters);
void uart_vUpDataHostMain_task(void *pvParameters);
void uart_vReceiveDataHostMain_task(void *pvParameters);

void app_main(void)
{
    flash_vFlashInit(&err_flash);

    button_vCreateInput(BUTTON_1, HIGH_TO_LOW);
    button_vCreateInput(BUTTON_2, HIGH_TO_LOW);
    button_vSetCallback(button_vEventCallback);

    output_vCreate(BLINK_GPIO);
    output_vCreate(LED_BLUE);
    output_vCreate(LED_GREEN);
    output_vCreate(LED_RED);

    DHT11_vInit(DHT_DATA);

    // Back up flash data
    flash_vFlashOpen(&err_flash, &my_handle_flash);
    u8Flag_run = flash_u8FlashReadU8(&err_flash, &my_handle_flash, &u8Flash_data);
    u8Flag_delay = u8Flag_run % 10;
    u8Flag_run /= 10;
    flash_vFlashClose(&my_handle_flash);

    xTaskCreate(dht11_vReadDataDht11_task, "dht11_vReadDataDht11_task", 2048, NULL, 5, NULL);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uart_vCreate();
    xTaskCreate(uart_vUpDataHostMain_task, "uart_vUpDataHostMain_task", 2048, NULL, 4, &xHandle);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uart_vCreate();
    xTaskCreate(uart_vReceiveDataHostMain_task, "uart_vReceiveDataHostMain_task", 2048, NULL, 3, NULL);
}

void button_vEventCallback(int pin)
{
    if (pin == BUTTON_1)
    {
        flash_vFlashOpen(&err_flash, &my_handle_flash);
        while (err_flash != ESP_OK)
            flash_vFlashOpen(&err_flash, &my_handle_flash);
        uint8_t u8Flag = flash_u8FlashReadU8(&err_flash, &my_handle_flash, &u8Flash_data);
        flash_vFlashClose(&my_handle_flash);
        u8Flag /= 10;
        if (u8Flag == SEND_UP_DATA_STATUS)
        {
            vTaskSuspend(xHandle);
        }
        else if (u8Flag == PAUSE_UP_DATA_STATUS)
        {
            vTaskResume(xHandle);
        }

        flash_vFlashOpen(&err_flash, &my_handle_flash);
        while (err_flash != ESP_OK)
            flash_vFlashOpen(&err_flash, &my_handle_flash);
        u8Flag_run = 1 - u8Flag_run;
        u8Flash_data = u8Flash_data % 10;
        u8Flash_data += 10 * u8Flag_run;
        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
    }
    else if (pin == BUTTON_2)
    {
        output_vToggle(BLINK_GPIO);
        flash_vFlashOpen(&err_flash, &my_handle_flash);
        while ((err_flash != ESP_OK))
            flash_vFlashOpen(&err_flash, &my_handle_flash);
        u8Flag_delay = u8Flash_data % 10;
        u8Flag_delay = (u8Flag_delay + 1) % 4;
        u8Flash_data = u8Flash_data / 10;
        u8Flash_data = u8Flash_data * 10 + u8Flag_delay;
        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
    }
}

void dht11_vReadDataDht11_task(void *pvParameters)
{
    for (;;)
    {
        if (DHT11_sRead().status == DHT11_OK)
        {
            u8Temperature = DHT11_sRead().temperature;
            u8Humidity = DHT11_sRead().humidity;

            if ((u8Temperature < LEVEL_TEMPERATURE) && (u8Humidity < LEVEL_HUMIDITY))
            {
                output_vSetLevel(LED_GREEN, LOW_LEVEL);
                output_vSetLevel(LED_BLUE, HIGH_LEVEL);
            }
            else if ((u8Temperature < LEVEL_TEMPERATURE) && (u8Humidity >= LEVEL_HUMIDITY))
            {
                output_vSetLevel(LED_BLUE, HIGH_LEVEL);
                output_vSetLevel(LED_GREEN, HIGH_LEVEL);
            }
            else if ((u8Temperature >= LEVEL_TEMPERATURE) && (u8Humidity < LEVEL_HUMIDITY))
            {
                output_vSetLevel(LED_BLUE, LOW_LEVEL);
                output_vSetLevel(LED_GREEN, HIGH_LEVEL);
            }
            else
            {
                output_vSetLevel(LED_RED, HIGH_LEVEL);
            }
        }
        vTaskDelay(FREQUENCY_1S / portTICK_PERIOD_MS);
    }
}

void uart_vUpDataHostMain_task(void *pvParameters)
{
    char chuoi_temp[FRAME_DATA_LENGTH] = {0x55, 0xaa, 0x00, COMMAND_WORD_TEMP, 0x00, 0x02};
    char chuoi_hum[FRAME_DATA_LENGTH] = {0x55, 0xaa, 0x00, COMMAND_WORD_HUMI, 0x00, 0x02};
    for (;;)
    {
        if (u8Flag_run == 0)
        {
            DHT_vConvertString(u8Temperature, chuoi_temp + 6);
            DHT_vCreateCheckSum(chuoi_temp);
            DHT_vConvertString(u8Humidity, chuoi_hum + 6);
            DHT_vCreateCheckSum(chuoi_hum);

            uart_write_bytes(EX_UART_NUM, chuoi_temp, FRAME_DATA_LENGTH);
            uart_write_bytes(EX_UART_NUM, chuoi_hum, FRAME_DATA_LENGTH);
        }
        else
        {
        }

        if (u8Flag_delay == TIME_DELAY_1S)
        {
            vTaskDelay(FREQUENCY_1S / portTICK_PERIOD_MS);
        }
        else if (u8Flag_delay == TIME_DELAY_5S)
        {
            vTaskDelay(FREQUENCY_5S / portTICK_PERIOD_MS);
        }
        else if (u8Flag_delay == TIME_DELAY_10S)
        {
            vTaskDelay(FREQUENCY_10S / portTICK_PERIOD_MS);
        }
        else
        {
            vTaskDelay(FREQUENCY_15S / portTICK_PERIOD_MS);
        }
    }
}

void uart_vReceiveDataHostMain_task(void *pvParameters)
{
    // printf("run in uart_vReceiveDataHostMain_task\n");
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE);
    // printf("run in uart_vReceiveDataHostMain_task\n");
    for (;;)
    {
        // Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            bzero(dtmp, RD_BUF_SIZE);
            switch (event.type)
            {
            // Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
            other types of events. If we take too much time on data event, the queue might
            be full.*/
            case UART_DATA:
                uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                uint8_t u8Check_sum = 0;
                for (uint8_t u8Index = 0; u8Index < FRAME_DATA_LENGTH - 1; u8Index++)
                    u8Check_sum = u8Check_sum + dtmp[u8Index];
                if (dtmp[2] == COMMAND_HOST_MAIN && u8Check_sum == dtmp[FRAME_DATA_LENGTH - 1])
                {
                    if (dtmp[4] == COMMAND_A)
                    {
                        flash_vFlashOpen(&err_flash, &my_handle_flash);
                        u8Flash_data = u8Flash_data % 10 + 10;
                        u8Flag_run = 1;
                        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
                        vTaskSuspend(xHandle);
                    }
                    else if (dtmp[4] == COMMAND_B)
                    {
                        flash_vFlashOpen(&err_flash, &my_handle_flash);
                        u8Flash_data = u8Flash_data % 10;
                        u8Flag_run = 0;
                        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
                        vTaskResume(xHandle);
                    }
                    else if (dtmp[4] == COMMAND_C)
                    {
                        uint8_t u8Time_delay_command = dtmp[7];
                        if (u8Time_delay_command == 0x01 || u8Time_delay_command == 0x05 || u8Time_delay_command == 0x0A || u8Time_delay_command == 0X0F)
                            output_vToggle(BLINK_GPIO);
                        else
                        {
                            break;
                        }
                        flash_vFlashOpen(&err_flash, &my_handle_flash);
                        switch (u8Time_delay_command)
                        {
                        case (FREQUENCY_1S / 1000):
                            u8Flag_delay = TIME_DELAY_1S;
                            break;
                        case (FREQUENCY_5S / 1000):
                            u8Flag_delay = TIME_DELAY_5S;
                            break;
                        case (FREQUENCY_10S / 1000):
                            u8Flag_delay = TIME_DELAY_10S;
                            break;
                        case (FREQUENCY_15S / 1000):
                            u8Flag_delay = TIME_DELAY_15S;
                        default:
                            break;
                        }
                        u8Flash_data /= 10;
                        u8Flash_data = u8Flash_data * 10 + u8Flag_delay;
                        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
                    }
                }
                else
                {
                }
                break;
            // Others
            default:
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}