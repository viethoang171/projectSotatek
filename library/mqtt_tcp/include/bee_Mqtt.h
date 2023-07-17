/**
 * @file bee_Mqtt.h
 * @author nguyen__viet_hoang
 * @date 05 July 2023
 * @brief module wifi, API init and config for module Mqtt to connect broker to publish and subcribe
 */
#ifndef BEE_MQTT_H
#define BEE_MQTT_H

#define MQTT_CONNECTED 1
#define MQTT_DISCONNECTED 0

#define BEE_PORT 1993
#define BEE_MQTT_BROKER "mqtt://61.28.238.97"
#define BEE_USER_NAME "VBeeHome"
#define BEE_PASS_WORD "123abcA@!"

#define LIMIT_TEMPERATURE 27
#define LIMIT_HUMIDITY 75

#define BEE_TIME_KEEP_ALIVE 15000
#define MQTT_PERIOD 30000

#define TIME_FOR_DELAY_TASK 10

#define FLAG_TEMPERATURE 1
#define FLAG_HUMIDITY 0

#define FLAG_WARNING 0
#define FLAG_NOT_WARNING 1

#define OBJECT_TYPE_TEMP "temperature"
#define OBJECT_TYPE_HUM "humidity"

#define SIZE_QUEUE_TASK_SUB 20

enum value_object_type
{
    BIT_LEVEL_TEMPERATURE = 0,
    BIT_LEVEL_HUMIDITY,
    BIT_AMPLITUDE_TEMPERATURE,
    BIT_AMPLITUDE_HUMIDITY,
    BIT_CANT_READ_DHT11
};

extern uint8_t u8Temperature;
extern uint8_t u8Humidity;

void mqtt_vApp_start();

void mqtt_vPublish_data_task(void *params);
void mqtt_vSubscribe_data_task(void *params);

#endif