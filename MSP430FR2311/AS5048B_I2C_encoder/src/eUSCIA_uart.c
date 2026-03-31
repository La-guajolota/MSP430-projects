#include "./include/eUSCIA_uart.h"

/**
 * @details
 */
void TXuart_send(uint8_t c){
    // Wait for the Transmit Interrupt Flag (UCTXIFG) to signal buffer is empty
    while(!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = c;                             // Move data to buffer; hardware handles shifting
}

/**
 * @details
 */
void uart_init(){
    /// UART pins
    P1SEL0 |= BIT7; 
    P1SEL1 &= ~BIT7;

    /* --- Protocol Configuration --- */
    UCA0CTLW0 |= UCSWRST;                      // Hold eUSCI in reset to modify registers
    UCA0CTLW0 |= UCSSEL__SMCLK;                // Source UART clock from Sub-Main Clock (16MHz)

    /* --- Baud Rate Calculation (Low Frequency Mode) ---
     * Clock Source (BRCLK) = 16,000,000 Hz
     * Desired Baud Rate = 115,200
     * Prescaler (N) = 16,000,000 / 115,200 = 138.88 > 8
     * UCBRx = INT(N/16) = 8
     * UCBRFx = INT([(N/16) – INT(N/16)] × 16) = 10
     * UCBRSx can be found by looking up the fractional part of N ( = N - INT(N) ) in table
     */
    UCA0BRW = 8;       
    UCA0MCTLW |= UCOS16;                       // UCOS16 (Oversampling) = 1 (Required since N > 16)
    UCA0MCTLW |= 0xA0;                         // UCBFx sists in [7:4]
    UCA0MCTLW |= 0xF700;                       // UCBRSx sits in high byte [15:8]

    UCA0CTLW0 &= ~UCSWRST;                     // Release reset to start UART state machine
}