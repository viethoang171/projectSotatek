/**
 * @file bee_Wifi.c
 * @author nguyen__viet_hoang
 * @date 14 August 2023
 * @brief module wifi, configure WiFi's credentials by pairing BLE
 */

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <wifi_provisioning/manager.h>

#include <wifi_provisioning/scheme_ble.h>

#include "bee_QrCode.h"
#include "bee_Mqtt.h"
#include "bee_Wifi.h"
#include "bee_FLash.h"

extern SemaphoreHandle_t xSemaphore;

static const char *TAG = "app";
static bool config_error = false;

static TickType_t last_time_time_out_config;
static int retries = 0;
static TaskHandle_t xHandle;

static char check_ssid[SSID_LENGTH];
static char check_password[PASS_WORD_LENGTH];

static void wifi_init_sta(void);
static void vCaculate_time_out_config_task()
{
    last_time_time_out_config = xTaskGetTickCount();
    for (;;)
    {
        if (xTaskGetTickCount() - last_time_time_out_config >= pdMS_TO_TICKS(TIME_OUT_CONFIG))
        {
            ESP_LOGI(TAG, "Time out config");
            wifi_prov_mgr_stop_provisioning();
            vTaskDelete(NULL);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

static void reconnect_corrected_wifi(void)
{
    wifi_config_t wifi_sta_cfg;

    char corrected_ssid[SSID_LENGTH];
    char corrected_password[PASS_WORD_LENGTH];

    flash_vGet_correct_wifi_credential(corrected_ssid, corrected_password);

    if (strlen(corrected_ssid) > 0) // If Wifi's credentials is configured
    {
        memset(&wifi_sta_cfg, 0, sizeof(wifi_config_t));
        strncpy((char *)wifi_sta_cfg.sta.ssid, corrected_ssid, sizeof(wifi_sta_cfg.sta.ssid));
        strncpy((char *)wifi_sta_cfg.sta.password, corrected_password, sizeof(wifi_sta_cfg.sta.password));

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_init_sta();
    }
}

static void vProcess_after_provision_fail_task()
{
    for (;;)
    {
        wifi_prov_mgr_reset_sm_state_on_failure();
        wifi_prov_mgr_stop_provisioning();
        reconnect_corrected_wifi();
        vTaskDelete(NULL);
    }
}

/* Event handler for catching system events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
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
            snprintf(check_ssid, sizeof(check_ssid), "%s", (const char *)wifi_sta_cfg->ssid);
            snprintf(check_password, sizeof(check_password), "%s", (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            config_error = true;
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            xTaskCreate(vProcess_after_provision_fail_task, "vProcess_after_provision_fail_task", 4096, NULL, 10, NULL);
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            flash_vSave_credential_wifi(check_ssid, check_password);
            config_error = false;
            ESP_LOGI(TAG, "Provisioning successful");
            mqtt_vApp_start();
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_stop_provisioning();
            break;
        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
            esp_wifi_connect();
            break;
        default:
            break;
        }
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

/* Handler for the optional provisioning endpoint registered by the application.
 * The data format can be chosen by applications. Here, we are using plain ascii text.
 * Applications can choose to use other formats like protobuf, JSON, XML, etc.
 */
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

static void vRetry_smart_config_task()
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

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
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

    /* Initialize provisioning manager with the
     * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    for (;;)
    {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
        {

            bool provisioned = false;

            /* Let's find out if the device is provisioned */
            ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

            /* If device is not yet provisioned start provisioning service */
            if (!provisioned || retries != 1)
            {

                ESP_LOGI(TAG, "Starting provisioning");

                /* What is the Device Service Name that we want
                 * This translates to :
                 *     - device name when scheme is wifi_prov_scheme_ble
                 */
                char service_name[12];
                get_device_service_name(service_name, sizeof(service_name));

                wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

                const char *proff_of_possess = "Bee@123";

                /* This is the structure for passing security parameters
                 * for the protocomm security 1.
                 */
                wifi_prov_security1_params_t *sec_params = proff_of_possess;

                /* What is the service key (could be NULL)
                 * This translates to :
                 *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
                 *          (Minimum expected length: 8, maximum 64 for WPA2-PSK)
                 *     - simply ignored when scheme is wifi_prov_scheme_ble
                 */
                const char *service_key = NULL;

                /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
                 * set a custom 128 bit UUID which will be included in the BLE advertisement
                 * and will correspond to the primary GATT service that provides provisioning
                 * endpoints as GATT characteristics. Each GATT characteristic will be
                 * formed using the primary service UUID as base, with different auto assigned
                 * 12th and 13th bytes (assume counting starts from 0th byte). The client side
                 * applications must identify the endpoints by reading the User Characteristic
                 * Description descriptor (0x2901) for each characteristic, which contains the
                 * endpoint name of the characteristic */
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

                /* If your build fails with linker errors at this point, then you may have
                 * forgotten to enable the BT stack or BTDM BLE settings in the SDK (e.g. see
                 * the sdkconfig.defaults in the example project) */
                wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

                /* An optional endpoint that applications can create if they expect to
                 * get some additional custom data during provisioning workflow.
                 * The endpoint name can be anything of your choice.
                 * This call must be made before starting the provisioning.
                 */
                wifi_prov_mgr_endpoint_create("custom-data");

                /* Do not stop and de-init provisioning even after success,
                 * so that we can restart it later. */

                /* Start provisioning service */
                ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *)sec_params, service_name, service_key));

                /* The handler for the optional endpoint created above.
                 * This call must be made after starting the provisioning, and only if the endpoint
                 * has already been created above.
                 */
                wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);

                if (xTaskGetHandle("caculate") == NULL)
                {
                    ESP_LOGI(TAG, "Create task time out");
                    xTaskCreate(vCaculate_time_out_config_task, "caculate", 2048, NULL, 1, NULL);
                }
            }
            else
            {
                if (config_error == true)
                {
                    wifi_prov_mgr_reset_sm_state_on_failure();
                }
                else
                {
                    mqtt_vApp_start();
                }
                ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

                /* Start Wi-Fi station */
                wifi_init_sta();
            }

            /* Start main application now */
            vTaskSuspend(NULL);
        }
    }
}

void wifi_vRetrySmartConfig()
{
    retries++;
    if (retries == 1)
        xTaskCreate(vRetry_smart_config_task, "vRetry_smart_config_task", 4096, NULL, 8, &xHandle);
    else
    {
        vTaskResume(xHandle);
    }
}
