/**
 * @file bee_Flash.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module for process with flash memory, API "init", "open", "close", "read", "write" for others functions and some macro for flag
 */
#ifndef BEE_FLASH_H
#define BEE_FLASH_H

#define ERROR_FLASH 6
#define ERROR_WRITE 0
#define WRITE_SUCCESS 1

#define TIME_DELAY_1S 0
#define TIME_DELAY_5S 1
#define TIME_DELAY_10S 2
#define TIME_DELAY_15S 3
#define SEND_UP_DATA_STATUS 0
#define PAUSE_UP_DATA_STATUS 1

void flash_vFlashInit(esp_err_t *pErr);
void flash_vFlashOpen(esp_err_t *pErr, nvs_handle_t *p_myHandle);
void flash_vFlashClose(nvs_handle_t *p_myHandle);
uint8_t flash_u8FlashReadU8(esp_err_t *pErr, nvs_handle_t *p_myHandle, uint8_t *pu8_Value);
uint8_t flash_u8FlashWriteU8(esp_err_t *pErr, nvs_handle_t *p_myHandle, uint8_t *pu8_Value);
#endif