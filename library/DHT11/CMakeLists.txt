set(component_srcs "source/bee_DHT.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver" "output"
                       REQUIRES "esp_timer")
