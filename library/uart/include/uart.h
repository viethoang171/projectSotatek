/**
 * @file uart.h
 * @author nguyen__viet_hoang
 * @date 25 June 2023
 * @brief module uart, API create for others functions, config Pins for UART
 */
#ifndef UART_H
#define UART_H
#define UART_TX GPIO_NUM_17
#define UART_RX GPIO_NUM_16

#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM (3) /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#define COMMAND_A "A"
#define COMMAND_B "B"
#define COMMAND_C "C"

void uart_vCreate();
#endif