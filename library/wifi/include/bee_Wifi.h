/**
 * @file bee_Wifi.h
 * @author nguyen__viet_hoang
 * @date 05 July 2023
 * @brief module wifi, API init and config for module WiFi to connect wifi
 */
#ifndef BEE_WIFI_H
#define BEE_WIFI_H

#define EXAMPLE_ESP_WIFI_SSID "BEE 2.4G"
// #define EXAMPLE_ESP_WIFI_SSID "Viethoang"
// #define EXAMPLE_ESP_WIFI_SSID "Bao Ngoc House T3"
#define EXAMPLE_ESP_WIFI_PASS "0988182139"
// #define EXAMPLE_ESP_WIFI_PASS "12345678"
// #define EXAMPLE_ESP_WIFI_PASS "0833138383"
#define MAX_RETRY 10

void wifi_vInit();

#endif