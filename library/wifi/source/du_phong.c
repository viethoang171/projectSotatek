/* Wi-Fi Provisioning Manager Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_bt.h>

#include <wifi_provisioning/manager.h>

#include <wifi_provisioning/scheme_ble.h>

#include "bee_QrCode.h"
#include "bee_Mqtt.h"
#include "bee_Wifi.h"

extern SemaphoreHandle_t xSemaphore;

static const char *TAG = "app";
static TickType_t last_time_time_out_config;
static int retries = 0;

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

bool provisioned = false;

/* Event handler for catching system events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    // static int retries;
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                          "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid,
                     (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            // retries++;
            // if (retries >= MAX_PROV_RETRY)
            // {
            ESP_LOGI(TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
            wifi_prov_mgr_reset_sm_state_on_failure();
            //     retries = 0;
            // }
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
        {
            ESP_LOGI(TAG, "Provisioning successful");
            // retries = 0;
            break;
        }
        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Deinit after finished provisioning");
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            provisioned = false;
            break;
        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
    }
}

static void wifi_init_sta(void)
{
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                   uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf)
    {
        ESP_LOGI(TAG, "Received data: %.*s", inlen, (char *)inbuf);
    }
    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL)
    {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
}

static void wifi_prov_print_qr(const char *name, const char *username, const char *pop, const char *transport)
{
    if (!name || !transport)
    {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150] = {0};
    if (pop)
    {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\""
                                           ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, pop, transport);
    }
    else
    {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\""
                                           ",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, transport);
    }

    ESP_LOGI(TAG, "Scan this QR code from the provisioning application for Provisioning.");
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&cfg, payload);

    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}

void smart_config(void)
{
    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);
}

static void vCaculate_time_out_config_task()
{
    last_time_time_out_config = xTaskGetTickCount();
    for (;;)
    {
        if (xTaskGetTickCount() - last_time_time_out_config >= pdMS_TO_TICKS(TIME_OUT_CONFIG))
        {
            ESP_LOGI(TAG, "Time out config");
            wifi_prov_mgr_stop_provisioning();
            ESP_LOGI(TAG, "After stop provisioning");
            vTaskDelete(NULL);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

static void vRetry_smart_config_task()
{
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
    {
        if (retries == 1)
        {
            /* Initialize NVS partition */
            esp_err_t ret = nvs_flash_init();
            if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
            {
                /* NVS partition was truncated
                 * and needs to be erased */
                ESP_ERROR_CHECK(nvs_flash_erase());

                /* Retry nvs_flash_init */
                ESP_ERROR_CHECK(nvs_flash_init());
            }

            /* Initialize TCP/IP */
            ESP_ERROR_CHECK(esp_netif_init());

            /* Initialize the event loop */
            ESP_ERROR_CHECK(esp_event_loop_create_default());
            wifi_event_group = xEventGroupCreate();

            /* Register our event handler for Wi-Fi, IP and Provisioning related events */
            ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
            ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
            ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

            /* Initialize Wi-Fi including netif with default config */
            esp_netif_create_default_wifi_sta();
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            /* Configuration for the provisioning manager */
            wifi_prov_mgr_config_t config = {
                .scheme = wifi_prov_scheme_ble,
                .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE};

            ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

            /* Let's find out if the device is provisioned */
            ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
            if (provisioned)
            {
                wifi_prov_mgr_deinit();
                wifi_init_sta();
                goto exit;
            }
            provisioned = true;
            char service_name[12];
            get_device_service_name(service_name, sizeof(service_name));

            wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

            const char *pop = "Bee@123";

            wifi_prov_security1_params_t *sec_params = pop;

            const char *username = NULL;

            const char *service_key = NULL;

            uint8_t custom_service_uuid[] = {
                /* LSB <---------------------------------------
                 * ---------------------------------------> MSB */
                0xb4,
                0xdf,
                0x5a,
                0x1c,
                0x3f,
                0x6b,
                0xf4,
                0xbf,
                0xea,
                0x4a,
                0x82,
                0x03,
                0x04,
                0x90,
                0x1a,
                0x02,
            };

            wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

            wifi_prov_mgr_endpoint_create("custom-data");

            ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *)sec_params, service_name, service_key));

            wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);

            xTaskCreate(vCaculate_time_out_config_task, "vCaculate_time_out_config_task", 4096, NULL, 1, NULL);

            wifi_prov_print_qr(service_name, username, pop, PROV_TRANSPORT_BLE);
        }
        else
        {
            /* Configuration for the provisioning manager */
            wifi_prov_mgr_config_t config = {
                .scheme = wifi_prov_scheme_ble,
                .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BLE};
            ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
            if (provisioned)
            {
                wifi_prov_mgr_deinit();
                wifi_init_sta();
                goto exit;
            }
            provisioned = true;
            ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
            char service_name[12];
            get_device_service_name(service_name, sizeof(service_name));

            wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

            const char *pop = "Bee@123";

            wifi_prov_security1_params_t *sec_params = pop;

            const char *username = NULL;

            const char *service_key = NULL;

            uint8_t custom_service_uuid[] = {
                /* LSB <---------------------------------------
                 * ---------------------------------------> MSB */
                0xb4,
                0xdf,
                0x5a,
                0x1c,
                0x3f,
                0x6b,
                0xf4,
                0xbf,
                0xea,
                0x4a,
                0x82,
                0x03,
                0x04,
                0x90,
                0x1a,
                0x02,
            };

            wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

            wifi_prov_mgr_endpoint_create("custom-data");
            ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *)sec_params, service_name, service_key));
            wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);
            xTaskCreate(vCaculate_time_out_config_task, "vCaculate_time_out_config_task", 4096, NULL, 1, NULL);
        }
    exit:

        smart_config();
        mqtt_vApp_start();
        vTaskDelete(NULL);
    }
}

void wifi_vRetrySmartConfig()
{
    retries++;
    xTaskCreate(vRetry_smart_config_task, "vRetry_smart_config_task", 4096, NULL, 8, NULL);
}