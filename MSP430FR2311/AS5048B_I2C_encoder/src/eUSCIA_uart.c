#include "./include/eUSCIA_uart.h"

void TXuart_send(uint8_t c){
    // Wait for the Transmit Interrupt Flag (UCTXIFG) to signal buffer is empty
    while(!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = c;                             
}

void uart_init(void){
    P1SEL0 |= BIT7; 
    P1SEL1 &= ~BIT7;

    UCA0CTLW0 |= UCSWRST;                      
    UCA0CTLW0 |= UCSSEL__SMCLK;                

    /* --- Baud Rate Calculation ---
     * Clock Source (BRCLK) = 16,000,000 Hz
     * Desired Baud Rate = 115,200
     * Prescaler (N) = 16,000,000 / 115,200 = 138.88 > 8
     * UCBRx = INT(N/16) = 8
     * UCBRFx = INT([(N/16) – INT(N/16)] × 16) = 10
     */
    UCA0BRW = 8;       
    UCA0MCTLW |= UCOS16;                       
    UCA0MCTLW |= 0xA0;                         
    UCA0MCTLW |= 0xF700;                       

    UCA0CTLW0 &= ~UCSWRST;                     
}