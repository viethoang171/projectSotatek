#include <stdio.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"
#include "flash.h"
void flash_init(esp_err_t *err)
{
    (*err) = nvs_flash_init();
    if ((*err) == ESP_ERR_NVS_NO_FREE_PAGES || (*err) == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        *err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(*err);
}
void flash_open(esp_err_t *err, nvs_handle_t *my_handle)
{
    *err = nvs_open("storage", NVS_READWRITE, my_handle);
}
uint8_t flash_read_u8(esp_err_t *err, nvs_handle_t *my_handle, uint8_t *value)
{
    *err = nvs_get_u8(*my_handle, "gia tri", value);

    switch (*err)
    {
    case ESP_OK:
        if (*value == 0)
            // printf("Read successfully!\n");
            return 0;
        else
            return 1;
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        return 2;
        // printf("The value is not initialized yet!\n");
        break;
    default:
        return 3;
        // printf("Error (%s) reading!\n", esp_err_to_name(*err));
    }
}
uint8_t flash_write_u8(esp_err_t *err, nvs_handle_t *my_handle, uint8_t *value)
{
    *err = nvs_set_u8(*my_handle, "gia tri", *value);
    uint8_t gia_tri = 0;
    if (*err == ESP_OK)
        gia_tri = 1;
    // printf((*err != ESP_OK) ? "Update Failed!\n" : "Update Success\n");
    *err = nvs_commit(*my_handle);
    // printf((*err != ESP_OK) ? "Commit Failed!\n" : "Commit Success\n");
    nvs_close(&my_handle);
    return gia_tri;
}