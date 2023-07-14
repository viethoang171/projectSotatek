/**
 * @file bee_Uart.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module uart, API create for others functions
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "bee_Led.h"
#include "bee_Button.h"
#include "bee_DHT.h"
#include "bee_FLash.h"
#include "bee_Uart.h"

QueueHandle_t uart0_queue;
char *TAG = "uart_event";

static TickType_t last_time_up_data_host_main;

extern SemaphoreHandle_t xSemaphore;

extern esp_err_t err_flash;
extern nvs_handle_t my_handle_flash;
extern TaskHandle_t xHandle;

static uint32_t uart_u32GetTimeDelayFromFlag(uint8_t u8Flag)
{
    uint32_t u32TimeDelay = 0;
    switch (u8Flag)
    {
    case TIME_DELAY_1S:
        u32TimeDelay = FREQUENCY_1S;
        break;
    case TIME_DELAY_5S:
        u32TimeDelay = FREQUENCY_5S;
        break;
    case TIME_DELAY_10S:
        u32TimeDelay = FREQUENCY_10S;
        break;
    case TIME_DELAY_15S:
        u32TimeDelay = FREQUENCY_15S;
        break;
    default:
        break;
    }
    return u32TimeDelay;
}

void uart_vCreate()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(GPIO_NUM_2, &uart_config);

    // Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    // Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(GPIO_NUM_2, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void uart_vUpDataHostMain_task(void *pvParameters)
{
    last_time_up_data_host_main = xTaskGetTickCount();
    char chuoi_temp[FRAME_DATA_LENGTH] = {0x55, 0xaa, 0x00, COMMAND_WORD_TEMP, 0x00, 0x02};
    char chuoi_hum[FRAME_DATA_LENGTH] = {0x55, 0xaa, 0x00, COMMAND_WORD_HUMI, 0x00, 0x02};
    for (;;)
    {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
        {
            if (xTaskGetTickCount() - last_time_up_data_host_main >= pdMS_TO_TICKS(uart_u32GetTimeDelayFromFlag(u8Flag_delay)))
            {
                last_time_up_data_host_main = xTaskGetTickCount();

                if (u8Flag_run == 0)
                {
                    DHT_vTransferFrameData(u8Temperature, chuoi_temp + 6);
                    DHT_vCreateCheckSum(chuoi_temp);
                    DHT_vTransferFrameData(u8Humidity, chuoi_hum + 6);
                    DHT_vCreateCheckSum(chuoi_hum);

                    uart_write_bytes(EX_UART_NUM, chuoi_temp, FRAME_DATA_LENGTH);
                    uart_write_bytes(EX_UART_NUM, chuoi_hum, FRAME_DATA_LENGTH);
                }

                vTaskDelay(TIME_FOR_DELAY_TASK / portTICK_PERIOD_MS);
            }
        }
    }
}

void uart_vReceiveDataHostMain_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *u8Data_from_host_main = (uint8_t *)malloc(RD_BUF_SIZE);
    for (;;)
    {
        // Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            switch (event.type)
            {
            // Event of UART receving data
            case UART_DATA:
                uart_read_bytes(EX_UART_NUM, u8Data_from_host_main, event.size, portMAX_DELAY);
                uint8_t u8Check_sum = 0;
                for (uint8_t u8Index = 0; u8Index < FRAME_DATA_LENGTH - 1; u8Index++)
                    u8Check_sum = u8Check_sum + u8Data_from_host_main[u8Index];
                if (u8Data_from_host_main[2] == COMMAND_HOST_MAIN && u8Check_sum == u8Data_from_host_main[FRAME_DATA_LENGTH - 1])
                {
                    if (u8Data_from_host_main[LOCATE_COMMAND_ID] == COMMAND_A)
                    {
                        flash_vFlashOpen(&err_flash, &my_handle_flash);
                        u8Flash_data = u8Flash_data % 10 + 10;
                        u8Flag_run = 1;
                        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
                        vTaskSuspend(xHandle);
                    }
                    else if (u8Data_from_host_main[LOCATE_COMMAND_ID] == COMMAND_B)
                    {
                        flash_vFlashOpen(&err_flash, &my_handle_flash);
                        u8Flash_data = u8Flash_data % 10;
                        u8Flag_run = 0;
                        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
                        vTaskResume(xHandle);
                    }
                    else if (u8Data_from_host_main[LOCATE_COMMAND_ID] == COMMAND_C)
                    {
                        uint8_t u8Time_delay_command = u8Data_from_host_main[LOCATE_TIME_DELAY];
                        if (u8Time_delay_command != 0x01 && u8Time_delay_command != 0x05 && u8Time_delay_command == 0x0A && u8Time_delay_command == 0X0F)
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

                break;
            // Others
            default:
                break;
            }
        }
    }
    free(u8Data_from_host_main);
    u8Data_from_host_main = NULL;
    vTaskDelete(NULL);
}
