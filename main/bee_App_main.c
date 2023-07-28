/**
 * @file bee_App_main.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for project read signal DHT11, up data to host main, receive signal from host main
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"
#include <driver/uart.h>
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

#include "bee_Button.h"
#include "bee_Led.h"
#include "bee_Uart.h"
#include "bee_DHT.h"
#include "bee_FLash.h"
#include "bee_Wifi.h"
#include "bee_Mqtt.h"

SemaphoreHandle_t xSemaphore;

TaskHandle_t xHandle;

esp_err_t err_flash;
nvs_handle_t my_handle_flash;
esp_err_t wifi_flash;

uint8_t u8Flash_data;
uint8_t u8Flag_delay = TIME_DELAY_1S;
uint8_t u8Flag_run = SEND_UP_DATA_STATUS;

void button_vEventCallback(uint8_t pin);

void app_main(void)
{
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore);

    flash_vFlashInit(&err_flash);

    button_vCreateInput(BUTTON_1, HIGH_TO_LOW);
    button_vCreateInput(BUTTON_2, HIGH_TO_LOW);
    button_vSetCallback(button_vEventCallback);

    output_vCreate(LED_BLUE);
    output_vCreate(LED_GREEN);
    output_vCreate(LED_RED);

    DHT11_vInit(DHT_DATA);
    uart_vCreate();

    flash_vFlashInit(&wifi_flash);
#if (0)
    esp_wifi_get_mac(ESP_IF_WIFI_STA, u8Mac_address);
#endif

    // Back up flash data
    flash_vFlashOpen(&err_flash, &my_handle_flash);
    u8Flag_run = flash_u8FlashReadU8(&err_flash, &my_handle_flash, &u8Flash_data);
    u8Flag_delay = u8Flag_run % 10;
    u8Flag_run /= 10;
    flash_vFlashClose(&my_handle_flash);

    xTaskCreate(mqtt_vPublish_data_task, "mqtt_vPublish_data_task", 1024 * 2, NULL, 5, NULL);
    xTaskCreate(mqtt_vSubscribe_data_task, "mqtt_vSubscribe_data_task", 1024 * 2, NULL, 6, NULL);
    xTaskCreate(dht11_vReadDataDht11_task, "dht11_vReadDataDht11_task", 1024, NULL, 2, NULL);
    xTaskCreate(uart_vUpDataHostMain_task, "uart_vUpDataHostMain_task", 1024, NULL, 4, &xHandle);
    xTaskCreate(uart_vReceiveDataHostMain_task, "uart_vReceiveDataHostMain_task", 1024 * 2, NULL, 3, NULL);

    smart_config();
    mqtt_vApp_start();
}

void button_vEventCallback(uint8_t pin)
{
    if (pin == BUTTON_1)
    {
        // Get flag run/pause value
        flash_vFlashOpen(&err_flash, &my_handle_flash);
        while (err_flash != ESP_OK)
            flash_vFlashOpen(&err_flash, &my_handle_flash);
        uint8_t u8Flag = flash_u8FlashReadU8(&err_flash, &my_handle_flash, &u8Flash_data);
        flash_vFlashClose(&my_handle_flash);
        u8Flag /= 10;

        // kiem tra flag run/pause
        if (u8Flag == SEND_UP_DATA_STATUS)
        {
            vTaskSuspend(xHandle);
        }
        else if (u8Flag == PAUSE_UP_DATA_STATUS)
        {
            vTaskResume(xHandle);
        }

        // Write flash cho flag luu run/pause up data status value
        flash_vFlashOpen(&err_flash, &my_handle_flash);
        while (err_flash != ESP_OK)
            flash_vFlashOpen(&err_flash, &my_handle_flash);

        u8Flag_run = 1 - u8Flag_run;
        u8Flash_data = u8Flash_data % 10 + 10 * u8Flag_run;

        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
    }
    else if (pin == BUTTON_2)
    {

        // Write flash cho flag luu time_delay up value for data status
        flash_vFlashOpen(&err_flash, &my_handle_flash);
        while ((err_flash != ESP_OK))
            flash_vFlashOpen(&err_flash, &my_handle_flash);

        u8Flag_delay = (u8Flash_data % 10 + 1) % 4;
        u8Flash_data = (u8Flash_data / 10) * 10 + u8Flag_delay;

        flash_u8FlashWriteU8(&err_flash, &my_handle_flash, &u8Flash_data);
    }
}