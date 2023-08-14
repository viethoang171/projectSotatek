/**
 * @file bee_FLash.c
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for process with flash memory, API "init", "open", "close", "read", "write" for others functions
 */
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bee_FLash.h"

uint8_t u8Flash_data;
uint8_t u8Flag_delay = TIME_DELAY_1S;
uint8_t u8Flag_run = SEND_UP_DATA_STATUS;

void flash_vFlashSaveStatus(esp_err_t *err_flash, nvs_handle_t *my_handle_flash)
{
    flash_vFlashOpen(err_flash, my_handle_flash);
    u8Flag_run = flash_u8FlashReadU8(err_flash, my_handle_flash, &u8Flash_data);
    u8Flag_delay = u8Flag_run % 10;
    u8Flag_run /= 10;
    flash_vFlashClose(my_handle_flash);
}

void flash_vFlashInit(esp_err_t *pErr)
{
    (*pErr) = nvs_flash_init();
    if ((*pErr) == ESP_ERR_NVS_NO_FREE_PAGES || (*pErr) == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        *pErr = nvs_flash_init();
    }
    ESP_ERROR_CHECK(*pErr);
}
void flash_vFlashOpen(esp_err_t *pErr, nvs_handle_t *p_myHandle)
{
    *pErr = nvs_open("storage", NVS_READWRITE, p_myHandle);
}
void flash_vFlashClose(nvs_handle_t *p_myHandle)
{
    nvs_close(p_myHandle);
}
uint8_t flash_u8FlashReadU8(esp_err_t *pErr, nvs_handle_t *p_myHandle, uint8_t *pu8_Value)
{
    *pErr = nvs_get_u8(*p_myHandle, "gia tri", pu8_Value);
    uint8_t u8GiaTri = *pu8_Value;

    switch (*pErr)
    {
    case ESP_OK:
        return (u8GiaTri);
        break;
    default:
        break;
    }
    return ERROR_FLASH;
}
uint8_t flash_u8FlashWriteU8(esp_err_t *pErr, nvs_handle_t *p_myHandle, uint8_t *pu8_Value)
{
    *pErr = nvs_set_u8(*p_myHandle, "gia tri", *pu8_Value);
    uint8_t u8GiaTri = ERROR_WRITE;
    if (*pErr == ESP_OK)
        u8GiaTri = WRITE_SUCCESS;
    *pErr = nvs_commit(*p_myHandle);
    nvs_close(&p_myHandle);
    return u8GiaTri;
}
void flash_vSaveDataButtonPause(esp_err_t *err_flash, nvs_handle_t *my_handle_flash, TaskHandle_t *xHandle)
{
    // Get flag run/pause value
    flash_vFlashOpen(err_flash, my_handle_flash);
    while (*err_flash != ESP_OK)
        flash_vFlashOpen(err_flash, my_handle_flash);
    uint8_t u8Flag = flash_u8FlashReadU8(err_flash, my_handle_flash, &u8Flash_data);
    flash_vFlashClose(my_handle_flash);
    u8Flag /= 10;

    // kiem tra flag run/pause
    if (u8Flag == SEND_UP_DATA_STATUS)
    {
        vTaskSuspend(*xHandle);
    }
    else if (u8Flag == PAUSE_UP_DATA_STATUS)
    {
        vTaskResume(*xHandle);
    }

    // Write flash cho flag luu run/pause up data status value
    flash_vFlashOpen(err_flash, my_handle_flash);
    while (*err_flash != ESP_OK)
        flash_vFlashOpen(err_flash, my_handle_flash);

    u8Flag_run = 1 - u8Flag_run;
    u8Flash_data = u8Flash_data % 10 + 10 * u8Flag_run;

    flash_u8FlashWriteU8(err_flash, my_handle_flash, &u8Flash_data);
}

void flash_vSaveDataButtonFrequency(esp_err_t *err_flash, nvs_handle_t *my_handle_flash)
{
    // Write flash cho flag luu time_delay up value for data status
    flash_vFlashOpen(err_flash, my_handle_flash);
    while ((*err_flash != ESP_OK))
        flash_vFlashOpen(err_flash, my_handle_flash);

    u8Flag_delay = (u8Flash_data % 10 + 1) % 4;
    u8Flash_data = (u8Flash_data / 10) * 10 + u8Flag_delay;

    flash_u8FlashWriteU8(err_flash, my_handle_flash, &u8Flash_data);
}

void flash_vSave_credential_wifi(const char *ssis, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open("wifi_credential", NVS_READWRITE, &nvs_handle);

    err = nvs_set_str(nvs_handle, "ssid_wifi", ssis);

    err = nvs_set_str(nvs_handle, "password_wifi", password);

    nvs_close(nvs_handle);
}

void flash_vGet_correct_wifi_credential(char *ssid, char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open("wifi_credential", NVS_READONLY, &nvs_handle);

    size_t ssid_len = SSID_LENGTH;
    err = nvs_get_str(nvs_handle, "ssid_wifi", ssid, &ssid_len);

    size_t password_len = PASS_WORD_LENGTH;
    err = nvs_get_str(nvs_handle, "password_wifi", password, &password_len);

    nvs_close(nvs_handle);
}