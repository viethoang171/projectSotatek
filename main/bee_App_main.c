/**
 * @file bee_App_main.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for project read signal DHT11, up data to host main, receive signal from host main
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"
#include "nvs_flash.h"

#include "bee_Button.h"
#include "bee_Led.h"
#include "bee_Uart.h"
#include "bee_DHT.h"
#include "bee_FLash.h"
#include "bee_Wifi.h"
#include "bee_Mqtt.h"

SemaphoreHandle_t xSemaphore;

TaskHandle_t xHandle;

esp_err_t err_flash, wifi_flash;
nvs_handle_t my_handle_flash;

void button_vEventCallback(uint8_t pin)
{
    if (pin == BUTTON_PAUSE_SEND)
    {
        flash_vSaveDataButtonPause(&err_flash, &my_handle_flash, &xHandle);
    }
    else if (pin == BUTTON_FREQUENCY)
    {
        flash_vSaveDataButtonFrequency(&err_flash, &my_handle_flash);
    }
    else if (pin == BUTTON_TIMEOUT)
    {
        wifi_vRetrySmartConfig();
    }
}

void app_main(void)
{
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore);

    flash_vFlashInit(&err_flash);

    button_vCreateInput(BUTTON_PAUSE_SEND, HIGH_TO_LOW);
    button_vCreateInput(BUTTON_FREQUENCY, HIGH_TO_LOW);
    button_vCreateInput(BUTTON_TIMEOUT, HIGH_TO_LOW);
    button_vSetCallback(button_vEventCallback);

    output_vCreate(LED_BLUE);
    output_vCreate(LED_GREEN);
    output_vCreate(LED_RED);
    output_vCreate(2);

    DHT11_vInit(DHT_DATA);
    uart_vCreate();

    flash_vFlashInit(&wifi_flash);
    flash_vFlashSaveStatus(&err_flash, &my_handle_flash);

    xTaskCreate(mqtt_vPublish_data_task, "mqtt_vPublish_data_task", 1024 * 4, NULL, 5, NULL);
    xTaskCreate(mqtt_vSubscribe_data_task, "mqtt_vSubscribe_data_task", 1024 * 2, NULL, 6, NULL);
    xTaskCreate(dht11_vReadDataDht11_task, "dht11_vReadDataDht11_task", 1024, NULL, 2, NULL);
    xTaskCreate(uart_vUpDataHostMain_task, "uart_vUpDataHostMain_task", 1024, NULL, 4, &xHandle);
    xTaskCreate(uart_vReceiveDataHostMain_task, "uart_vReceiveDataHostMain_task", 1024 * 2, NULL, 3, NULL);
}
