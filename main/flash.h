#ifndef FLASH_H
#define FLASH_H
void flash_init(esp_err_t *err);
void flash_open(esp_err_t *err, nvs_handle_t *my_handle);
uint8_t flash_read_u8(esp_err_t *err, nvs_handle_t *my_handle, uint8_t *value);
uint8_t flash_write_u8(esp_err_t *err, nvs_handle_t *my_handle, uint8_t *value);
#endif