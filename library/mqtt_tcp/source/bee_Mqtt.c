/**
 * @file bee_Wifi.c
 * @author nguyen__viet_hoang
 * @date 05 July 2023
 * @brief module wifi, API for config and init wifi to connect
 */
#include "esp_log.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"
#include "bee_DHT.h"
#include "bee_Mqtt.h"

uint8_t u8Mac_address[6] = {0xb8, 0xd6, 0x1a, 0x6b, 0x2d, 0xe8};
char topic_subscribe[100];

// static TickType_t last_time_send_keep_alive;
extern SemaphoreHandle_t xSemaphore;
extern uint8_t u8Count_error_dht_signal;

static const char *TAG = "MQTT_EXAMPLE";
static esp_mqtt_client_handle_t client = NULL;
static uint32_t u8Mqtt_status = MQTT_DISCONNECTED;

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
        u8Mqtt_status = MQTT_CONNECTED;

        msg_id = esp_mqtt_client_subscribe(client, "VB/DMP/VBEEON/CUSTOM/SMH/b8d61a6b2de8/telemetry", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        u8Mqtt_status = MQTT_DISCONNECTED;
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

void mqtt_vApp_start()
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

void mqtt_vPublish_data_task(void *params)
{
    // last_time_send_keep_alive = xTaskGetTickCount();
    char data_temp[200];
    char data_hum[200];
    char message_keep_alive[200];
    uint8_t u8Trans_code = 0;

    snprintf(topic_subscribe, sizeof(topic_subscribe), "VB/DMP/VBEEON/CUSTOM/SMH/b8d61a6b2de8/Command");
    for (;;)
    {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
        {
            // if (xTaskGetTickCount() - last_time_send_keep_alive > pdMS_TO_TICKS(MQTT_PERIOD))
            // {
            // last_time_send_keep_alive = xTaskGetTickCount();
            if (DHT11_sRead().status == DHT11_OK)
            {
                u8Temperature = DHT11_sRead().temperature;
                u8Humidity = DHT11_sRead().humidity;

                // Create message JSON keep alive
                snprintf(message_keep_alive, sizeof(message_keep_alive),
                         "{\"think_token\":\"b8d61a6b2de8\","
                         "\"values\":{"
                         "\"eventType\":\"refresh\","
                         "\"status\":\"ONLINE\""
                         "},"
                         "\"trans_code:\"%d\""
                         "}",
                         u8Trans_code);

                // Create message JSON publish temperature
                snprintf(data_temp, sizeof(data_temp),
                         "{\"think_token\":\"%02x%02x%02x%02x%02x%02x\","
                         "\"cmd_name\":\"Bee.data\","
                         "\"object_type\":\"Bee.temperature\","
                         "\"values\":\"%d\","
                         "\"trans_code\":\"%d\"}",
                         u8Mac_address[0], u8Mac_address[1], u8Mac_address[2], u8Mac_address[3], u8Mac_address[4], u8Mac_address[5], u8Temperature, u8Trans_code);

                // Create message JSON publish humidity
                snprintf(data_hum, sizeof(data_hum),
                         "{\"think_token\":\"%02x%02x%02x%02x%02x%02x\","
                         "\"cmd_name\":\"Bee.data\","
                         "\"object_type\":\"Bee.humidity\","
                         "\"values\":\"%d\","
                         "\"trans_code\":\"%d\"}",
                         u8Mac_address[0], u8Mac_address[1], u8Mac_address[2], u8Mac_address[3], u8Mac_address[4], u8Mac_address[5], u8Humidity, u8Trans_code);

                if (u8Mqtt_status == MQTT_CONNECTED)
                {
                    u8Trans_code++;
                    esp_mqtt_client_publish(client, topic_subscribe, data_temp, 0, 0, 0);
                    u8Trans_code++;
                    esp_mqtt_client_publish(client, topic_subscribe, data_hum, 0, 0, 0);
                    u8Trans_code++;
                    esp_mqtt_client_publish(client, topic_subscribe, message_keep_alive, 0, 0, 0);
                }
            }
            else
            {
                if (u8Mqtt_status == MQTT_CONNECTED)
                {
                    if (u8Count_error_dht_signal >= 5)
                        esp_mqtt_client_publish(client, topic_subscribe, "Measure error!", 0, 0, 0);
                }
            }
            // }
            vTaskDelay(MQTT_PERIOD / portTICK_PERIOD_MS);
        }
    }
}