/**
 * @file bee_DHT.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for read signal DHT11, API "init", "read", "convert format signal" for others functions
 */

#include "esp_timer.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "bee_Led.h"
#include "bee_DHT.h"

extern SemaphoreHandle_t xSemaphore;
static TickType_t last_time_read_dht;
static uint8_t u8Flag_first_time_read = 0;

static gpio_num_t dht_gpio;
static int64_t i64Last_read_time = -2000000;
static struct dht11_reading sLast_read;

uint8_t u8Amplitude_temperature;
uint8_t u8Amplitude_humidity;

uint8_t u8Count_error_dht_signal = 0;
uint8_t u8Count_times_dht_data_change = 0;
uint8_t u8String_temperature[MAX_NUM_DATA] = {0};
uint8_t u8String_humidity[MAX_NUM_DATA] = {0};
uint16_t u16Start_period_data = 1;
uint16_t u16End_period_data = 1;

static uint32_t DHT_u32WaitOrTimeOut(uint16_t u16MicroSeconds, uint8_t u8Level)
{
    uint32_t u32Micros_ticks = 0;
    while (gpio_get_level(dht_gpio) == u8Level)
    {
        if (u32Micros_ticks++ > u16MicroSeconds)
            return DHT11_TIMEOUT_ERROR;
        ets_delay_us(1);
    }
    return u32Micros_ticks;
}

static int DHT_i32CheckCRC(uint8_t data[])
{
    if (data[4] == (data[0] + data[1] + data[2] + data[3]))
        return DHT11_OK;
    else
        return DHT11_CRC_ERROR;
}

static void DHT_vSendStartSignal()
{
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    ets_delay_us(20 * 1000);
    gpio_set_level(dht_gpio, 1);
    ets_delay_us(40);
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);
}

static int DHT_i32CheckResponse()
{
    /* Wait for next step ~80us*/
    if (DHT_u32WaitOrTimeOut(80, 0) == DHT11_TIMEOUT_ERROR)
        return DHT11_TIMEOUT_ERROR;

    /* Wait for next step ~80us*/
    if (DHT_u32WaitOrTimeOut(80, 1) == DHT11_TIMEOUT_ERROR)
        return DHT11_TIMEOUT_ERROR;

    return DHT11_OK;
}

static struct dht11_reading DHT_sTimeOutError()
{
    struct dht11_reading timeoutError = {DHT11_TIMEOUT_ERROR, -1, -1};
    return timeoutError;
}

static struct dht11_reading DHT_sCrcError()
{
    struct dht11_reading crcError = {DHT11_CRC_ERROR, -1, -1};
    return crcError;
}

void DHT11_vInit(uint8_t gpio_num)
{
    /* Wait 1 seconds to make the device pass its initial unstable status */
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    dht_gpio = gpio_num;
}

struct dht11_reading DHT11_sRead()
{
    /* Tried to sense too son since last read (dht11 needs ~2 seconds to make a new read) */
    if (esp_timer_get_time() - 2000000 < i64Last_read_time)
    {
        return sLast_read;
    }

    i64Last_read_time = esp_timer_get_time();

    uint8_t data[5] = {0, 0, 0, 0, 0};

    DHT_vSendStartSignal();

    if (DHT_i32CheckResponse() == DHT11_TIMEOUT_ERROR)
        return sLast_read = DHT_sTimeOutError();

    /* Read response */
    for (uint8_t i = 0; i < 40; i++)
    {
        /* Initial data */
        if (DHT_u32WaitOrTimeOut(50, 0) == DHT11_TIMEOUT_ERROR)
            return sLast_read = DHT_sTimeOutError();

        if (DHT_u32WaitOrTimeOut(70, 1) > 28)
        {
            /* Bit received was a 1 */
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    if (DHT_i32CheckCRC(data) != DHT11_CRC_ERROR)
    {
        sLast_read.status = DHT11_OK;
        sLast_read.temperature = data[2];
        sLast_read.humidity = data[0];
        return sLast_read;
    }
    else
    {
        return sLast_read = DHT_sCrcError();
    }
}

static uint8_t DHT_u8CaculateAmplitude(uint16_t u16Start, uint16_t u16End, uint8_t u8String_data[])
{
    uint8_t u8Amplitude;
    uint16_t u8SumValueToCalculateAverage = 0;
    if (u16Start < u16End && u16End < MAX_NUM_DATA)
    {
        printf("-------------");
        for (uint16_t u16Index = u16Start + 1; u16Index < u16End; u16Index++)
        {
            printf("%d ", u8String_data[u16Index]);
            u8SumValueToCalculateAverage += u8String_data[u16Index];
        }
        printf("---------\n");
    }
    else
    {
        printf("-------------");
        for (uint16_t u16Index = u16Start + 1; u16Index < MAX_NUM_DATA; u16Index++)
        {
            printf("%d ", u8String_data[u16Index]);
            u8SumValueToCalculateAverage += u8String_data[u16Index];
        }

        for (uint16_t u16Index = 1; u16Index < u16End; u16Index++)
        {
            printf("%d ", u8String_data[u16Index]);
            u8SumValueToCalculateAverage += u8String_data[u16Index];
        }
        printf("---------\n");
    }

    if (u8SumValueToCalculateAverage > (TIMES_CHECK_LEVEL - 1) * u8String_data[u16End])
    {
        u8Amplitude = u8SumValueToCalculateAverage - (TIMES_CHECK_LEVEL - 1) * u8String_data[u16End];
    }
    else
    {
        u8Amplitude = (TIMES_CHECK_LEVEL - 1) * u8String_data[u16End] - u8SumValueToCalculateAverage;
    }
    return u8Amplitude;
}
static void DHT_vSaveData()
{
    // Neu doc lan dau
    if (u8Flag_first_time_read == 0)
    {
        u8String_temperature[u16End_period_data] = u8Temperature;
        u8String_humidity[u16End_period_data] = u8Humidity;
        if (u8Count_times_dht_data_change == TIMES_CHECK_LEVEL)
        {
            u8Flag_first_time_read = 1;

            u8Amplitude_temperature = DHT_u8CaculateAmplitude(u16Start_period_data, u16End_period_data, u8String_temperature);
            u8Amplitude_humidity = DHT_u8CaculateAmplitude(u16Start_period_data, u16End_period_data, u8String_humidity);

            u16Start_period_data++;
            u8Count_times_dht_data_change = 1;
        }
        else
        {
            u8Count_times_dht_data_change++;
        }
        u16End_period_data++;
    }
    else
    {
        if (u16Start_period_data == MAX_NUM_DATA)
        {
            u16Start_period_data = 1;
        }

        if (u16End_period_data == MAX_NUM_DATA)
        {
            u16End_period_data = 1;
        }

        u8String_temperature[u16End_period_data] = u8Temperature;
        u8String_humidity[u16End_period_data] = u8Humidity;

        if (u8Count_times_dht_data_change == TIMES_CHECK_LEVEL)
        {
            u8Amplitude_temperature = DHT_u8CaculateAmplitude(u16Start_period_data, u16End_period_data, u8String_temperature);
            u8Amplitude_humidity = DHT_u8CaculateAmplitude(u16Start_period_data, u16End_period_data, u8String_humidity);
            u8Count_times_dht_data_change = 1;
        }

        else
        {
            u8Count_times_dht_data_change++;
        }

        u16Start_period_data++;
        u16End_period_data++;
    }
}

void DHT_vTransferFrameData(uint8_t u8Data, char *chuoi)
{
    uint8_t u8Length_string = 0;
    char chuoi_luu[5];
    if (u8Data == 0)
    {
        chuoi[u8Length_string++] = 0;
        chuoi[u8Length_string] = 0;
        return;
    }

    chuoi_luu[u8Length_string++] = u8Data;
    chuoi_luu[u8Length_string] = 0;
    for (uint8_t u8Index = 0; u8Index <= u8Length_string; u8Index++)
        chuoi[u8Index] = chuoi_luu[u8Length_string - u8Index];
}
void DHT_vConvertString(uint8_t u8Data, char *chuoi)
{
    uint8_t u8Length_string = 0;
    char chuoi_luu[5];
    if (u8Data == 0)
    {
        chuoi[u8Length_string++] = '0';
        chuoi[u8Length_string] = '0';
        return;
    }
    while (u8Data != 0)
    {
        chuoi_luu[u8Length_string++] = (char)(u8Data % 10 + 48);
        u8Data /= 10;
    }
    for (uint8_t u8Index = 0; u8Index <= u8Length_string; u8Index++)
        chuoi[u8Index] = chuoi_luu[u8Length_string - u8Index];
}

void DHT_vCreateCheckSum(char *chuoi)
{
    uint8_t u8CheckSum = 0;
    for (uint8_t i = 0; i < FRAME_DATA_LENGTH - 1; i++)
        u8CheckSum = u8CheckSum + chuoi[i];
    chuoi[FRAME_DATA_LENGTH - 1] = u8CheckSum;
}

void dht11_vReadDataDht11_task(void *pvParameters)
{
    last_time_read_dht = xTaskGetTickCount();
    for (;;)
    {
        if (xTaskGetTickCount() - last_time_read_dht >= pdMS_TO_TICKS(TIME_READ_DHT))
        {
            last_time_read_dht = xTaskGetTickCount();
            if (DHT11_sRead().status == DHT11_OK)
            {
                u8Count_error_dht_signal = 0;
                u8Temperature = DHT11_sRead().temperature;
                u8Humidity = DHT11_sRead().humidity;
                output_vSetWarningLed(u8Temperature, u8Humidity);

                // Luu lai temperature and humidity
                DHT_vSaveData();
            }
            else
            {
                u8Temperature = VALUE_MEASURE_DHT_ERROR;
                u8Humidity = VALUE_MEASURE_DHT_ERROR;
                u8Count_error_dht_signal++;
            }
        }
        xSemaphoreGive(xSemaphore);
        vTaskDelay(TIME_FOR_READ_DHT / portTICK_PERIOD_MS);
    }
}
