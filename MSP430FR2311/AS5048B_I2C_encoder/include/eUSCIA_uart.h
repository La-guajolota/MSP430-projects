/**
 * @file eUSCIA_uart.h
 * @brief Hardware Abstraction Layer for eUSCI_A UART communication.
 * @author Adrián Silva Palafox
 * @date may 31 2026
 */
#ifndef eUSCIA_uart_h
#define eUSCIA_uart_h

#include <msp430.h>
#include <stdint.h>

/**
 * @brief Executes a blocking UART transmission of a single byte.
 * * Polls the UCTXIFG hardware flag to ensure the transmit buffer is empty 
 * before loading the new character.
 * * @param c 8-bit data character to transmit.
 */
void TXuart_send(uint8_t c);

/**
 * @brief Initializes eUSCI_A0 for asynchronous UART at 115200 Baud.
 * * Configures GPIO routing and calculates hardware prescalers assuming a 
 * 16 MHz Sub-Main Clock (SMCLK) source. Utilizes Low-Frequency oversampling mode.
 */
void uart_init(void);

#endif