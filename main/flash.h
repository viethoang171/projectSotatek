#ifndef FLASH_H
#define FLASH_H
#define TIME_DELAY_1S 0
#define TIME_DELAY_5S 1
#define TIME_DELAY_10S 2
#define TIME_DELAY_15S 3
#define SEND_UP_DATA_STATUS 0
#define PAUSE_UP_DATA_STATUS 1
void flash_init(esp_err_t *err);
void flash_open(esp_err_t *err, nvs_handle_t *my_handle);
uint8_t flash_read_u8(esp_err_t *err, nvs_handle_t *my_handle, uint8_t *value);
uint8_t flash_write_u8(esp_err_t *err, nvs_handle_t *my_handle, uint8_t *value);
#endif