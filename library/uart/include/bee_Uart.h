/**
 * @file bee_Uart.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module uart, API create for others functions, config Pins for UART
 */
#ifndef BEE_UART_H
#define BEE_UART_H
#define UART_TX GPIO_NUM_17
#define UART_RX GPIO_NUM_16

#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM (3) /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#define COMMAND_A 0x0A
#define COMMAND_B 0x0B
#define COMMAND_C 0x0C
#define COMMAND_HOST_MAIN 0x03

void uart_vCreate();
#endif