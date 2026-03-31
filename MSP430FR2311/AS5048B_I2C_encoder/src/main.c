/**
 * @file main.c
 * @brief Demonstration application for the legacy 16-bit MSP430 microcontroller family.
 * * This file implements a bare-metal data acquisition system. It utilizes the eUSCI_B0 
 * module to interface with an AS5048B magnetic position sensor via I2C. To maintain 
 * real-time determinism and avoid floating-point software emulation penalties on 
 * the 16-bit RISC architecture, raw 14-bit data is extracted and offloaded to a host 
 * machine via the eUSCI_A0 UART module for subsequent processing.
 * @author Adrián Silva Palafox
 * @date may 31 2026
 * * @note Architecture: MSP430FR2xxx (16-bit RISC, no hardware FPU).
 */

#include <msp430.h> 
#include <stdint.h>
#include <stdbool.h>

#include "include/utils.h"
#include "include/eUSCIA_uart.h"
#include "include/AS5048B.h"

/** * @brief Hardware abstraction instance for the eUSCI_B0 I2C module.
 */
I2C_port_t I2C_comm;

/** * @brief Driver instance for the AS5048B magnetic position sensor.
 */
AS5048B_Driver_t encoder;

/** @brief Default 7-bit I2C address for the AS5048B sensor. */
#define ENCODER_ID 0x40

/** @brief Logical index assigned to the primary encoder instance. */
#define ENCODER_NUM_ID 0X00

/** @brief Synchronization header byte for the UART framing protocol. */
#define UART_FRAME_SYNC 0xAA

/**
 * @brief Main execution entry point.
 * * Configures the system clock to 16 MHz, initializes peripheral modules (UART and I2C), 
 * and registers the AS5048B sensor. The primary execution loop polls the sensor for 
 * 16-bit raw angular data and streams it serially using a 4-byte frame format at 
 * a deterministic 10ms interval.
 * * @return void
 */
void main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    initClockTo16MHz();
    uart_init();
    I2C_port_init(&I2C_comm);

    if (AS5048B_Init(&encoder, &I2C_comm) == IDLE_MODE) {
        AS5048B_AddDevice(&encoder, ENCODER_NUM_ID, ENCODER_ID);
        AS5048B_SetZeroPosition(&encoder, ENCODER_NUM_ID);
    }

    while (true) { 
        uint16_t raw_angle = AS5048B_GetAngle16bit(&encoder, ENCODER_NUM_ID);
        
        // Transmit 4-byte standardized frame for Host-side floating-point evaluation
        TXuart_send(UART_FRAME_SYNC);
        TXuart_send((uint8_t)(raw_angle >> 8));
        TXuart_send((uint8_t)(raw_angle & 0xFF));
        TXuart_send('\n');

        // Deterministic 10ms sampling rate execution (160,000 cycles at 16 MHz)
        __delay_cycles(160000); 
    }
}

//******************************************************************************
// I2C Interrupt ***************************************************************
//******************************************************************************

/**
 * @brief eUSCI_B0 Interrupt Service Routine (ISR).
 * * Executes the hardware state machine for I2C Master communication. 
 * Evaluates the UCB0IV interrupt vector to handle acknowledgment failures (NACK), 
 * byte reception (RX), and byte transmission (TX). The intrinsic function 
 * __even_in_range is utilized to generate a deterministic jump table, ensuring 
 * O(1) execution time for state transitions.
 */
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCI_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  //Must read from UCB0RXBUF
  uint8_t rx_val = 0;
  
  switch(__even_in_range(UCB0IV, USCI_I2C_UCBIT9IFG)) {
    case USCI_NONE:          break;         // Vector 0: No interrupts
    case USCI_I2C_UCALIFG:   break;         // Vector 2: ALIFG

    case USCI_I2C_UCNACKIFG:                // Vector 4: NACKIFG
      I2C_comm.I2C_stMachine = NACK_MODE;
      UCB0CTLW0 |= UCTXSTP;                 // Send stop condition
      UCB0IFG &= ~UCNACKIFG;                // Clear flag manually
      break;
    
    case USCI_I2C_UCSTTIFG:  break;         // Vector 6: STTIFG
    case USCI_I2C_UCSTPIFG:  break;         // Vector 8: STPIFG
    case USCI_I2C_UCRXIFG3:  break;         // Vector 10: RXIFG3
    case USCI_I2C_UCTXIFG3:  break;         // Vector 12: TXIFG3
    case USCI_I2C_UCRXIFG2:  break;         // Vector 14: RXIFG2
    case USCI_I2C_UCTXIFG2:  break;         // Vector 16: TXIFG2
    case USCI_I2C_UCRXIFG1:  break;         // Vector 18: RXIFG1
    case USCI_I2C_UCTXIFG1:  break;         // Vector 20: TXIFG1
    
    case USCI_I2C_UCRXIFG0:                 // Vector 22: RXIFG0
      rx_val = UCB0RXBUF;
      if (I2C_comm.RXByteCtr)
      {
        I2C_comm.ReceiveBuffer[I2C_comm.ReceiveIndex++] = rx_val;
        I2C_comm.RXByteCtr--;
      }

      if (I2C_comm.RXByteCtr == 1)
      {
        UCB0CTLW0 |= UCTXSTP;
      }
      else if (I2C_comm.RXByteCtr == 0)
      {
        UCB0IE &= ~UCRXIE;
        I2C_comm.I2C_stMachine = IDLE_MODE;
      }
        break;

    case USCI_I2C_UCTXIFG0:                 // Vector 24: TXIFG0
      switch (I2C_comm.I2C_stMachine) {
        case TX_REG_ADDRESS_MODE:
          UCB0TXBUF = I2C_comm.TransmitRegAddr;
          if (I2C_comm.RXByteCtr) {
            I2C_comm.I2C_stMachine = SWITCH_TO_RX_MODE;   // Need to start receiving now
          } else {
            I2C_comm.I2C_stMachine = TX_DATA_MODE;        // Continue to transmision with the data in Transmit Buffer
          }
          break;
        case SWITCH_TO_RX_MODE:
          UCB0IE |= UCRXIE;                               // Enable RX interrupt
          UCB0IE &= ~UCTXIE;                              // Disable TX interrupt
          UCB0CTLW0 &= ~UCTR;                             // Switch to receiver
          I2C_comm.I2C_stMachine = RX_DATA_MODE;          // State state is to receive data
          UCB0CTLW0 |= UCTXSTT;                           // Send repeated start
          
          /* * WARNING: Blocking delay within an ISR.
           * Stalls the CPU until the START condition generation completes.
           */
          if (I2C_comm.RXByteCtr == 1) {
            //Must send stop since this is the N-1 byte
            while((UCB0CTLW0 & UCTXSTT));
            UCB0CTLW0 |= UCTXSTP;                         // Send stop condition
          }
          break;

        case TX_DATA_MODE:
          if (I2C_comm.TXByteCtr)
          {
            UCB0TXBUF = I2C_comm.TransmitBuffer[I2C_comm.TransmitIndex++];
            I2C_comm.TXByteCtr--;
          }
          else
          {
            //Done with transmission
            UCB0CTLW0 |= UCTXSTP;                         // Send stop condition
            I2C_comm.I2C_stMachine = IDLE_MODE;
            UCB0IE &= ~UCTXIE;                            // disable TX interrupt
          }
          break;
        
        /*** USED FOR THE I2C_Master_IsSlaveAvailable() ***/
        case SWITCH_TO_SLAVE_ACK:
          I2C_comm.I2C_stMachine = ACK_MODE;
          UCB0IE &= ~UCTXIE;                              // Disable TX interrupt to prevent re-entry
          UCB0CTLW0 |= UCTXSTP;                           // Assert STOP condition
          UCB0TXBUF = 0x00;                               // Write dummy byte to clear SCL stall
          break;

        default:
          __no_operation();
          break;
      }
      break;
    default: break;
  }
}