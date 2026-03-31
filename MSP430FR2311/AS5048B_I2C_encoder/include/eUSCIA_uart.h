#ifndef eUSCIA_uart_h
#define eUSCIA_uart_h

/*** Includes ***/
#include <msp430.h>
#include <stdint.h>

/**
 * @brief Polling-based UART Transmission.
 * @param c 8-bit data/character to send.
 */
void TXuart_send(uint8_t c);

/**
 * @brief Initializes eUSCI_A0 for UART 115200 Baud @ 1MHz SMCLK.
 */
void uart_init();

#endif