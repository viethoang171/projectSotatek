set(component_srcs "source/bee_Mqtt.c" )

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "mqtt" "DHT11" "cJson"
                       REQUIRES "")