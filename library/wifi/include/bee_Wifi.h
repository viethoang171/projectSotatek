/**
 * @file bee_Wifi.h
 * @author nguyen__viet_hoang
 * @date 28 July 2023
 * @brief module wifi, API init and config for module WiFi to connect wifi
 */
#ifndef SMART_CONFIG_H
#define SMART_CONFIG_H

#define MAX_PROV_RETRY 5
#define TIME_OUT_CONFIG 60000

#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_BLE "ble"
#define QRCODE_BASE_URL "https://espressif.github.io/esp-jumpstart/qrcode.html"

void wifi_vRetrySmartConfig();

#endif