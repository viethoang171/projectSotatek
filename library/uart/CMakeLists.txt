set(component_srcs "source/bee_Uart.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver" "output" "DHT11" "button" "flash" "nvs_flash" "cJson"
                       REQUIRES "")