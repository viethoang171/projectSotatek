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
#include "bee_cJSON.h"
#include "bee_DHT.h"
#include "bee_Mqtt.h"

extern SemaphoreHandle_t xSemaphore;
extern uint8_t u8Count_error_dht_signal;
extern uint8_t u8Amplitude_temperature;
extern uint8_t u8Amplitude_humidity;
extern uint8_t u8Count_times_dht_data_change;

static uint8_t u8Mac_address[6] = {0xb8, 0xd6, 0x1a, 0x6b, 0x2d, 0xe8};
static char topic_subscribe[100];
static char mac_address[20];

static const char *TAG = "MQTT_EXAMPLE";
static esp_mqtt_client_handle_t client = NULL;
static uint32_t u8Mqtt_status = MQTT_DISCONNECTED;

static TickType_t last_time_send_keep_alive;
static TickType_t last_time_send_data_sensor;

static uint8_t u8Trans_code = 0;

static struct sMessage_json
{
    char *data_temperature;
    char *data_humidity;
    char *keep_alive;
} sMessage_Json;

static struct sWarning_problem
{
    uint8_t warning;
    uint8_t timeout_warning;
    uint8_t flag_warnned;
} sWarning_read, sWarning_level_temp, sWarning_amplitude_temp, sWarning_amplitude_hum;

static QueueHandle_t queue;
static char rxBuffer[500];

static void mqtt_vCreate_message_json_keep_alive();
static void mqtt_check_error_to_publish_warning();
static void mqtt_vCreate_message_json_data(char *string_value, uint8_t u8CheckWarning, uint8_t u8Flag_timeout_warning);

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

        snprintf(rxBuffer, event->data_len + 1, event->data);
        xQueueSend(queue, rxBuffer, (TickType_t)0);
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

void mqtt_vSubscribe_data_task(void *params)
{
    queue = xQueueCreate(SIZE_QUEUE_TASK_SUB, sizeof(rxBuffer));
    for (;;)
    {
        if (xQueueReceive(queue, &rxBuffer, (TickType_t)portMAX_DELAY))
        {

            cJSON *root = cJSON_Parse(rxBuffer);
            if (root != NULL)
            {
                u8Trans_code++;
                char *device_id = cJSON_GetObjectItemCaseSensitive(root, "thing_token")->valuestring;
                char *cmd_name = cJSON_GetObjectItemCaseSensitive(root, "cmd_name")->valuestring;
                char *object_type = cJSON_GetObjectItemCaseSensitive(root, "object_type")->valuestring;

                if (strcmp(device_id, mac_address) == 0 && strcmp(cmd_name, "Bee.conf") == 0)
                {
                    if (strcmp(object_type, OBJECT_TYPE_TEMP) == 0 && (u8Mqtt_status == MQTT_CONNECTED))
                    {
                        sMessage_Json.data_temperature = "Number temp";
                        mqtt_vCreate_message_json_data(sMessage_Json.data_temperature, FLAG_NOT_WARNING, FLAG_NOT_WARNING);
                    }
                    else if (strcmp(object_type, OBJECT_TYPE_HUM) == 0 && (u8Mqtt_status == MQTT_CONNECTED))
                    {
                        sMessage_Json.data_temperature = "Number hum";
                        mqtt_vCreate_message_json_data(sMessage_Json.data_humidity, FLAG_NOT_WARNING, FLAG_NOT_WARNING);
                    }
                }
                cJSON_Delete(root);
            }
        }
    }
}

void mqtt_vPublish_data_task(void *params)
{
    last_time_send_keep_alive = xTaskGetTickCount();
    last_time_send_data_sensor = last_time_send_keep_alive;

    snprintf(mac_address, sizeof(mac_address), "%02x%02x%02x%02x%02x%02x", u8Mac_address[0], u8Mac_address[1], u8Mac_address[2], u8Mac_address[3], u8Mac_address[4], u8Mac_address[5]);
    snprintf(topic_subscribe, sizeof(topic_subscribe), "VB/DMP/VBEEON/CUSTOM/SMH/%s/Command", mac_address);
    for (;;)
    {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
        {
            // Xu ly warning len server
            if (u8Mqtt_status == MQTT_CONNECTED)
            {
                mqtt_check_error_to_publish_warning();
            }

            // publish message keep alive
            if (xTaskGetTickCount() - last_time_send_keep_alive > pdMS_TO_TICKS(BEE_TIME_KEEP_ALIVE))
            {

                last_time_send_keep_alive = xTaskGetTickCount();
                u8Trans_code++;

                mqtt_vCreate_message_json_keep_alive();
                esp_mqtt_client_publish(client, topic_subscribe, sMessage_Json.keep_alive, 0, 0, 0);
            }

            // publish message data temp, hum to server
            if (xTaskGetTickCount() - last_time_send_data_sensor > pdMS_TO_TICKS(MQTT_PERIOD))
            {

                last_time_send_data_sensor = xTaskGetTickCount();
                if (DHT11_sRead().status == DHT11_OK)
                {
                    u8Temperature = DHT11_sRead().temperature;
                    u8Humidity = DHT11_sRead().humidity;

                    if (u8Mqtt_status == MQTT_CONNECTED)
                    {
                        sMessage_Json.data_temperature = "Number temp";
                        sMessage_Json.data_humidity = "Number hum";
                        mqtt_vCreate_message_json_data(sMessage_Json.data_temperature, FLAG_NOT_WARNING, FLAG_NOT_WARNING);
                        mqtt_vCreate_message_json_data(sMessage_Json.data_humidity, FLAG_NOT_WARNING, FLAG_NOT_WARNING);
                    }
                }
            }
            else
            {
                xSemaphoreGive(xSemaphore);
            }
            vTaskDelay(TIME_FOR_DELAY_TASK / portTICK_PERIOD_MS);
        }
    }
}

static void mqtt_vCreate_message_json_keep_alive()
{
    cJSON *root, *values;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "thing_token", cJSON_CreateString(mac_address));
    cJSON_AddItemToObject(root, "values", values = cJSON_CreateObject());
    cJSON_AddItemToObject(values, "eventType", cJSON_CreateString("refresh"));
    cJSON_AddItemToObject(values, "status", cJSON_CreateString("ONLINE"));
    cJSON_AddItemToObject(root, "trans_code", cJSON_CreateNumber(u8Trans_code));
    sMessage_Json.keep_alive = cJSON_Print(root);
    cJSON_Delete(root);
}
static void mqtt_vCreate_message_json_data(char *string_value, uint8_t u8CheckWarning, uint8_t u8Flag_timeout_warning)
{
    char *message_json_publish;
    u8Trans_code++;

    cJSON *root;
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "thing_token", cJSON_CreateString(mac_address));
    cJSON_AddItemToObject(root, "cmd_name", cJSON_CreateString("Bee.data"));

    // Edit object_type
    if (strcmp(string_value, "Number temp") != 0 && strcmp(string_value, "Number hum") != 0)
    {
        cJSON_AddItemToObject(root, "object_type", cJSON_CreateString("Bee_warning"));
    }
    else if (strcmp(string_value, "Number temp") == 0)
    {
        cJSON_AddItemToObject(root, "object_type", cJSON_CreateString("temperature"));
    }
    else if (strcmp(string_value, "Number hum") == 0)
    {
        cJSON_AddItemToObject(root, "object_type", cJSON_CreateString("humidity"));
    }

    // Edit values
    if (strcmp(string_value, "Number temp") != 0 && strcmp(string_value, "Number hum") != 0)
    {
        cJSON_AddItemToObject(root, "values", cJSON_CreateString(string_value));
    }
    else if (strcmp(string_value, "Number temp") == 0)
    {
        cJSON_AddItemToObject(root, "values", cJSON_CreateNumber(u8Temperature));
    }
    else if (strcmp(string_value, "Number hum") == 0)
    {
        cJSON_AddItemToObject(root, "values", cJSON_CreateNumber(u8Humidity));
    }

    cJSON_AddItemToObject(root, "trans_code", cJSON_CreateNumber(u8Trans_code));
    message_json_publish = cJSON_Print(root);
    cJSON_Delete(root);

    if (u8CheckWarning == FLAG_WARNING && u8Flag_timeout_warning != FLAG_WARNING)
        esp_mqtt_client_publish(client, topic_subscribe, message_json_publish, 0, 0, 0);
    else if (u8CheckWarning == FLAG_NOT_WARNING)
    {
        esp_mqtt_client_publish(client, topic_subscribe, message_json_publish, 0, 0, 0);
    }
}

static void mqtt_check_error_to_publish_warning()
{
    char string_warning_key_value[60];

    if (u8Count_error_dht_signal >= 5 && sWarning_read.warning == FLAG_WARNING)
    {
        sWarning_read.warning = FLAG_NOT_WARNING;
        sWarning_read.timeout_warning = FLAG_WARNING;
        snprintf(string_warning_key_value, sizeof(string_warning_key_value), "can't read DHT11 signal");
        mqtt_vCreate_message_json_data(string_warning_key_value, FLAG_NOT_WARNING, FLAG_NOT_WARNING);

        sWarning_read.flag_warnned = 1;
    }
    if (u8Count_error_dht_signal == 1 && sWarning_read.timeout_warning == FLAG_WARNING)
    {
        sWarning_read.timeout_warning = FLAG_NOT_WARNING;
        sWarning_read.warning = FLAG_WARNING;
        snprintf(string_warning_key_value, sizeof(string_warning_key_value), "timeout warning can't read DHT11");
        mqtt_vCreate_message_json_data(string_warning_key_value, FLAG_WARNING, sWarning_read.flag_warnned);
    }

    if (u8Temperature > LIMIT_TEMPERATURE && u8Temperature != VALUE_MEASURE_DHT_ERROR && sWarning_level_temp.warning == FLAG_WARNING)
    {
        sWarning_level_temp.warning = FLAG_NOT_WARNING;
        sWarning_level_temp.timeout_warning = FLAG_WARNING;
        snprintf(string_warning_key_value, sizeof(string_warning_key_value), "%d over 38*C", u8Temperature);
        mqtt_vCreate_message_json_data(string_warning_key_value, FLAG_NOT_WARNING, FLAG_NOT_WARNING);

        sWarning_level_temp.flag_warnned = 1;
    }
    if (u8Temperature <= LIMIT_TEMPERATURE && u8Temperature != VALUE_MEASURE_DHT_ERROR && sWarning_level_temp.timeout_warning == FLAG_WARNING)
    {
        sWarning_level_temp.timeout_warning = FLAG_NOT_WARNING;
        sWarning_level_temp.warning = FLAG_WARNING;
        snprintf(string_warning_key_value, sizeof(string_warning_key_value), "timeout warning over level temp");

        mqtt_vCreate_message_json_data(string_warning_key_value, FLAG_WARNING, sWarning_level_temp.flag_warnned);
    }

    if (u8Count_times_dht_data_change == TIMES_CHECK_LEVEL)
    {
        if (u8Amplitude_temperature > TIMES_CHECK_LEVEL && sWarning_amplitude_temp.warning == FLAG_WARNING)
        {
            sWarning_amplitude_temp.warning = FLAG_NOT_WARNING;
            sWarning_amplitude_temp.timeout_warning = FLAG_WARNING;
            snprintf(string_warning_key_value, sizeof(string_warning_key_value), "Temperature amplitude %f over 2*C", (float)(u8Amplitude_temperature / 49));
            mqtt_vCreate_message_json_data(string_warning_key_value, FLAG_NOT_WARNING, FLAG_NOT_WARNING);

            sWarning_amplitude_temp.flag_warnned = 1;
        }
        if (u8Amplitude_temperature <= TIMES_CHECK_LEVEL && sWarning_amplitude_temp.timeout_warning == FLAG_WARNING)
        {
            sWarning_amplitude_temp.timeout_warning = FLAG_NOT_WARNING;
            sWarning_amplitude_temp.warning = FLAG_WARNING;
            snprintf(string_warning_key_value, sizeof(string_warning_key_value), "timeout warning amplitude temperature");

            mqtt_vCreate_message_json_data(string_warning_key_value, FLAG_WARNING, sWarning_amplitude_temp.flag_warnned);
        }
        if (u8Amplitude_humidity > TIMES_CHECK_LEVEL && sWarning_amplitude_hum.warning == FLAG_WARNING)
        {
            sWarning_amplitude_hum.warning = FLAG_NOT_WARNING;
            sWarning_amplitude_hum.timeout_warning = FLAG_WARNING;
            snprintf(string_warning_key_value, sizeof(string_warning_key_value), "Humidity amplitude %f over 2", (float)(u8Amplitude_humidity / 49));
            mqtt_vCreate_message_json_data(string_warning_key_value, FLAG_NOT_WARNING, FLAG_NOT_WARNING);

            sWarning_amplitude_hum.flag_warnned = 1;
        }
        if (u8Amplitude_humidity <= TIMES_CHECK_LEVEL && sWarning_amplitude_hum.timeout_warning == FLAG_WARNING)
        {
            sWarning_amplitude_hum.timeout_warning = FLAG_NOT_WARNING;
            sWarning_amplitude_hum.warning = FLAG_WARNING;
            snprintf(string_warning_key_value, sizeof(string_warning_key_value), "timeout warning amplitude humidity");

            mqtt_vCreate_message_json_data(string_warning_key_value, FLAG_WARNING, sWarning_amplitude_hum.flag_warnned);
        }
    }
}