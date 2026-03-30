#include <msp430.h> 
#include <stdint.h>
#include <stdbool.h>
#include "include/AS5048B.h"

void initClockTo16MHz();

I2C_port_t I2C_comm;
AS5048B_Driver_t encoder;

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;	     // Stop watchdog timer

    initClockTo16MHz();
    
    I2C_port_init(&I2C_comm);

    AS5048B_Init(&encoder, &I2C_comm);
    AS5048B_AddDevice(&encoder, 1, 0x40);
    AS5048B_UpdateRegisters(&encoder, 1);

    while (true) { 
      __no_operation();
    }
}

void initClockTo16MHz() {
    // Configure one FRAM waitstate as required by the device datasheet for MCLK operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    // Clock System Setup
    __bis_SR_register(SCG0);                           // disable FLL
    CSCTL3 |= SELREF__REFOCLK;                         // Set REFO as FLL reference source
    CSCTL0 = 0;                                        // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                            // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_5;                               // Set DCO = 16MHz
    CSCTL2 = FLLD_0 + 487;                             // DCOCLKDIV = 16MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                           // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));         // FLL locked

    // Disable the GPIO power-on default high-impedance mode to activate previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;           
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