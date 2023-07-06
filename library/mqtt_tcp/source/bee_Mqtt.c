/**
 * @file bee_Wifi.c
 * @author nguyen__viet_hoang
 * @date 05 July 2023
 * @brief module wifi, API for config and init wifi to connect
 */
#include "esp_log.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "bee_DHT.h"
#include "bee_Mqtt.h"

static const char *TAG = "MQTT_EXAMPLE";
static esp_mqtt_client_handle_t client = NULL;
static uint32_t MQTT_CONNECTED = 0;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        MQTT_CONNECTED = 1;

        msg_id = esp_mqtt_client_subscribe(client, "VB/DMP/VBEEON/CUSTOM/SMH/DeviceID/telemetry", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test2", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        MQTT_CONNECTED = 0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start()
{
    ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqttConfig = {
        .broker.address.uri = BEE_MQTT_BROKER,
        .broker.address.port = BEE_PORT,
        .session.keepalive = BEE_TIME_KEEP_ALIVE,
        .credentials.username = BEE_USER_NAME,
        .credentials.authentication.password = BEE_PASS_WORD};

    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void Publisher_Task(void *params)
{
    char chuoi_temp[12] = {'N', 'h', 'i', 'e', 't', ' ', 'd', 'o', ':', ' '};
    char chuoi_hum[9] = {'D', 'o', ' ', 'a', 'm', ':', ' '};

    for (;;)
    {
        if (DHT11_sRead().status == DHT11_OK)
        {
            u8Temperature = DHT11_sRead().temperature;
            u8Humidity = DHT11_sRead().humidity;
        }

        DHT_vConvertString(u8Temperature, chuoi_temp + 10);
        DHT_vConvertString(u8Humidity, chuoi_hum + 7);

        if (MQTT_CONNECTED)
        {
            esp_mqtt_client_publish(client, "VB/DMP/VBEEON/CUSTOM.SMH/DeviceID/Command", chuoi_temp, 0, 0, 0);
            esp_mqtt_client_publish(client, "VB/DMP/VBEEON/CUSTOM.SMH/DeviceID/Command", chuoi_hum, 0, 0, 0);
        }
        vTaskDelay(MQTT_PERIOD / portTICK_PERIOD_MS);
    }
}