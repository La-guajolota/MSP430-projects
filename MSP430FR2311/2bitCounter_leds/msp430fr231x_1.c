/**
 * @file main.c
 * @brief 2-Bit Binary Counter with UART Telemetry for MSP430FR Series
 * @author Adrián Silva Palafox
 * @date May 14th 2026
 * * @details 
 * This implementation uses a 1MHz SMCLK to drive a UART interface at 115200 baud.
 * It maps a software counter to physical GPIOs using a HAL-style struct.
 * Clock: Default MCLK/SMCLK ~1MHz (DCO)
 * UART Config: 8N1 (8 Data bits, No Parity, 1 Stop Bit), LSB First
 */

#include <msp430.h>
#include <stdint.h>

//#define SMCLK_P1_0 // Uncomment to output the internal SMCLK signal for frequency verification

/**
 * @struct LED_Config_t
 * @brief Hardware abstraction layer for GPIO mapping.
 * Provides a pointer-based method to toggle bits without hardcoding port names in logic.
 */
typedef struct {
    volatile uint8_t* port_out; ///< Pointer to Output Register (e.g., &P1OUT)
    uint8_t pin_mask;           ///< Bitmask for the specific pin (e.g., BIT0)
    uint8_t counter_shift;      ///< Bit position in the system counter to monitor
} LED_Config_t;

/**
 * @brief Updates a physical LED state based on the system counter.
 * @param val Current 8-bit counter value.
 * @param led Pointer to the specific LED configuration.
 */
void update_led_state(uint8_t val, const LED_Config_t* led) {
    // Check if the bit at 'counter_shift' is high in the 'val'
    if (val & (1 << led->counter_shift)) {
        *(led->port_out) |= led->pin_mask;   // Set bit high
    } else {
        *(led->port_out) &= ~(led->pin_mask); // Clear bit low
    }
}

/**
 * @brief Initializes eUSCI_A0 for UART 115200 Baud @ 1MHz SMCLK.
 */
void uart_init(){
    /* --- GPIO Muxing ---
     * P1SEL bits configure the pin for "Primary/Secondary Peripheral" mode.
     * Note: Verify if UCA0TXD is on P1.7 for your specific MSP430 model.
     */
    P1SEL0 |= BIT7; 
    P1SEL1 &= ~BIT7;

    /* --- Protocol Configuration --- */
    UCA0CTLW0 |= UCSWRST;                      // Hold eUSCI in reset to modify registers
    UCA0CTLW0 |= UCSSEL__SMCLK;                // Source UART clock from Sub-Main Clock (1MHz)

    /* --- Baud Rate Calculation (Low Frequency Mode) ---
     * Clock Source (BRCLK) = 1,000,000 Hz
     * Desired Baud Rate = 115,200
     * Prescaler (N) = 1,000,000 / 115,200 = 8.68
     * * UCBRx (Integer part) = 8
     * UCBRSx (Fractional part 0.68) = 0xD6 (Refer to MSP430 User Guide Table)
     * UCOS16 (Oversampling) = 0 (Required since N < 16)
     */
    UCA0BRW = 8;                               
    UCA0MCTLW = 0xD600;                        // UCBRSx sits in high byte [15:8]

    UCA0CTLW0 &= ~UCSWRST;                     // Release reset to start UART state machine
}

/**
 * @brief Polling-based UART Transmission.
 * @param c 8-bit data/character to send.
 */
void TXuart_send(uint8_t c){
    // Wait for the Transmit Interrupt Flag (UCTXIFG) to signal buffer is empty
    while(!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = c;                             // Move data to buffer; hardware handles shifting
}

int main(void) {
    /* --- 1. System Initialization --- */
    WDTCTL = WDTPW | WDTHOLD;                  // Stop watchdog timer

    uart_init();                               // Setup UART

    /* --- 2. GPIO Setup --- */
    P1DIR |= BIT0;                             // P1.0 as Output
#ifdef SMCLK_P1_0    
    // Configures P1.0 to output the actual SMCLK signal for oscilloscope testing
    P1SEL0 &= ~BIT0;                          
    P1SEL1 |= BIT0;
#endif
    P2DIR |= BIT0;                             // P2.0 as Output

    /* --- 3. Power Management Unlock ---
     * MSP430FR (FRAM) devices power up with I/O locked in high-impedance mode.
     * This bit must be cleared to activate the configured GPIO settings.
     */
    PM5CTL0 &= ~LOCKLPM5;

    /* --- 4. Logic Definitions --- */
    const LED_Config_t led_msb = { &P1OUT, BIT0, 1 }; // LED 1 monitors Bit 1
    const LED_Config_t led_lsb = { &P2OUT, BIT0, 0 }; // LED 2 monitors Bit 0

    uint8_t system_counter = 0;

    /* --- 5. Main Loop --- */
    while(1) {
#ifndef SMCLK_P1_0
        update_led_state(system_counter, &led_msb);
#endif
        update_led_state(system_counter, &led_lsb);

        /* Note: Sending raw binary (0, 1, 2...) may result in non-printable 
         * characters in your terminal. Use (system_counter + '0') for ASCII digits. */
        TXuart_send(system_counter);
        
        system_counter++;                      // Increment counter (rolls over at 255)

        // Hardware delay: 500,000 cycles / 1,000,000 Hz = ~0.5 Seconds
        __delay_cycles(500000);
    }
}