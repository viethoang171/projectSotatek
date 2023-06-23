/* UART Events Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "uart.h"
#include "output_iot.h"

extern QueueHandle_t uart0_queue;
extern char *TAG;
//  extern TaskHandle_t xHandle;
//  extern bool flag_run;
//  extern int flag_delay;
void uart_create()
{
   uart_config_t uart_config = {
       .baud_rate = 115200,
       .data_bits = UART_DATA_8_BITS,
       .parity = UART_PARITY_DISABLE,
       .stop_bits = UART_STOP_BITS_1,
       .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
       .source_clk = UART_SCLK_DEFAULT,
   };
   esp_log_level_set(TAG, ESP_LOG_INFO);

   // Install UART driver, and get the queue.
   uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
   uart_param_config(EX_UART_NUM, &uart_config);

   // Set UART log level
   esp_log_level_set(TAG, ESP_LOG_INFO);
   // Set UART pins (using UART0 default pins ie no changes.)
   uart_set_pin(UART_NUM_2, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

   // Set uart pattern detect function.
   uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
   // Reset the pattern queue length to record at most 20 pattern positions.
   uart_pattern_queue_reset(EX_UART_NUM, 20);
}
