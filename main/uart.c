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
#define COMMAND_A "A"
#define COMMAND_B "B"
#define COMMAND_C "C"
static QueueHandle_t uart0_queue;
static char *TAG = "uart_events";
extern TaskHandle_t xHandle;
extern bool flag_run;
extern int flag_delay;
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

void uart_receive_data_host_main(void *pvParameters)
{
   printf("run in uart_receive_data_host_main\n");
   uart_event_t event;
   uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE);
   for (;;)
   {
      // Waiting for UART event.
      printf("run in UART event\n");
      if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY))
      {
         bzero(dtmp, RD_BUF_SIZE);
         ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
         switch (event.type)
         {
         // Event of UART receving data
         /*We'd better handler data event fast, there would be much more data events than
         other types of events. If we take too much time on data event, the queue might
         be full.*/
         case UART_DATA:
            // ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
            uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
            if (strcmp((char *)dtmp, COMMAND_A) == 0)
            {
               flag_run = 1;
               vTaskSuspend(xHandle);
            }
            else if (strcmp((char *)dtmp, COMMAND_B) == 0)
            {
               flag_run = 0;
               vTaskResume(xHandle);
            }
            else if (strcmp((char *)dtmp, COMMAND_C) == 0)
            {
               flag_delay = (flag_delay + 1) % 4;
            }
            break;
         // Others
         default:
            ESP_LOGI(TAG, "uart event type: %d", event.type);
            break;
         }
      }
   }
   free(dtmp);
   dtmp = NULL;
   vTaskDelete(NULL);
}
