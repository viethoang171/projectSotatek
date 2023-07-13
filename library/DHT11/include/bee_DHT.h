/**
 * @file bee_DHT.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for read signal DHT11, API "init", "read", "convert format signal" for others functions and some macro config Pins, level temperature and humidity
 */

#ifndef BEE_DHT_H_
#define BEE_DHT_H_

#define DHT_DATA GPIO_NUM_4
#define MAX_NUM_DATA 1001

#define FRAME_DATA_LENGTH 9
#define TIME_READ_DHT 1000

#define COMMAND_WORD_TEMP 0x01
#define COMMAND_WORD_HUMI 0X02

#define TIME_FOR_READ_DHT 200
#define VALUE_MEASURE_DHT_ERROR 255

#define TIMES_CHECK_LEVEL 50

extern uint8_t u8Temperature;
extern uint8_t u8Humidity;
extern uint8_t u8Status;

enum dht11_status
{
    DHT11_CRC_ERROR = -2,
    DHT11_TIMEOUT_ERROR,
    DHT11_OK
};

struct dht11_reading
{
    int status;
    int temperature;
    int humidity;
};

void DHT11_vInit(uint8_t gpio_num);

struct dht11_reading DHT11_sRead();

void DHT_vTransferFrameData(uint8_t u8Data, char *chuoi);

void DHT_vCreateCheckSum(char *chuoi);

void dht11_vReadDataDht11_task(void *pvParameters);

void DHT_vConvertString(uint8_t u8Data, char *chuoi);

#endif