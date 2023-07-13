/**
 * @file bee_Wifi.c
 * @author nguyen__viet_hoang
 * @date 05 July 2023
 * @brief module wifi, API for config and init wifi to connect
 */
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_system.h"

#include "bee_Mqtt.h"
#include "bee_Wifi.h"

static uint8_t u8Retry_connect = 0;
static const char *TAG = "WIFI";

static esp_err_t wifi_espEvent_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "Trying to connect with Wi-Fi\n");
        break;

    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "Wi-Fi connected\n");
        break;

    case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip: starting MQTT Client\n");
        mqtt_vApp_start();
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "disconnected: Retrying Wi-Fi\n");
        if (u8Retry_connect++ < MAX_RETRY)
        {
            esp_wifi_connect();
        }
        else
            ESP_LOGI(TAG, "Max Retry Failed: Wi-Fi Connection\n");
        break;

    default:
        break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return ESP_OK;
}

void wifi_vInit()
{
    esp_event_loop_create_default();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_espEvent_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_espEvent_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_netif_init();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}
