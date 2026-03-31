#include <msp430.h> 
#include <stdint.h>
#include <stdbool.h>

#include "include/utils.h"
#include "include/eUSCIA_uart.h"
#include "include/AS5048B.h"

I2C_port_t I2C_comm;
AS5048B_Driver_t encoder;

#define ENCODER_ID 0x40
#define ENCODER_NUM_ID 0X00
#define UART_FRAME_SYNC 0xAA

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

        // Deterministic 10ms sampling rate execution
        __delay_cycles(160000); 
    }
}

//******************************************************************************
// I2C Interrupt ***************************************************************
//******************************************************************************

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