/**
 * @file bee_Mqtt.h
 * @author nguyen__viet_hoang
 * @date 05 July 2023
 * @brief module wifi, API init and config for module Mqtt to connect broker to publish and subcribe
 */
#ifndef BEE_MQTT_H
#define BEE_MQTT_H

#define BEE_PORT 1993
#define BEE_MQTT_BROKER "mqtt://61.28.238.97"
#define BEE_USER_NAME "VBeeHome"
#define BEE_PASS_WORD "123abcA@!"
#define BEE_TIME_KEEP_ALIVE 15

#define MQTT_PERIOD 30000

extern uint8_t u8Temperature;
extern uint8_t u8Humidity;

void mqtt_app_start();

void Publisher_Task(void *params);

#endif