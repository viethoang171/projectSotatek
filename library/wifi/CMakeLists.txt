set(component_srcs "source/bee_Wifi.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "esp_wifi" "esp_netif" "mqtt_tcp" "nvs_flash" "wifi_provisioning"
                       REQUIRES "")