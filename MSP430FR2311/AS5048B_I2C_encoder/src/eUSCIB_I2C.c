#include "./include/eUSCIB_I2C.h"

/**
* @details
*/
I2C_Mode_t I2C_Master_ReadReg(I2C_port_t* comm, uint8_t dev_addr, uint8_t reg_addr, uint8_t count) {
    // Wait for bus to be idle before initiating START condition
    while (UCB0STATW & UCBBUSY);
    
    /* Initialize state machine */
    comm->I2C_stMachine = TX_REG_ADDRESS_MODE;
    comm->TransmitRegAddr = reg_addr;
    comm->RXByteCtr = count;
    comm->TXByteCtr = 0;
    comm->ReceiveIndex = 0;
    comm->TransmitIndex = 0;

    /* Initialize slave address stage */
    UCB0I2CSA = dev_addr;                    // Slave Address
    /* Initialize ISR*/
    UCB0IFG &= ~(UCTXIFG + UCRXIFG);         // Clear any pending interrupts
    UCB0IE &= ~UCRXIE;                       // Disable RX interrupt
    UCB0IE |= UCTXIE;                        // Enable TX interrupt

    UCB0CTLW0 |= UCTR + UCTXSTT;             // SDA transmit mode, generte a start condition
    __enable_interrupt();

    // Block until ISR modifies the state machine variables
    while (!(comm->I2C_stMachine == IDLE_MODE));

    // Ensure STOP condition is fully clocked out to the physical bus before returning
    while (UCB0CTLW0 & UCTXSTP);

    return comm->I2C_stMachine;
}

/**
* @details
*/
I2C_Mode_t I2C_Master_WriteReg(I2C_port_t* comm, uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count) {
    // Wait for bus to be idle before initiating START condition
    while (UCB0STATW & UCBBUSY);
    
    /* Initialize state machine */
    comm->I2C_stMachine = TX_REG_ADDRESS_MODE;
    comm->TransmitRegAddr = reg_addr;

    /** 
    * Copy register data to TransmitBuffer
    * @todo Implement memcpy by DMA instead if available  
    */
    memcpy(&(comm->TransmitBuffer), reg_data, count);

    comm->TXByteCtr = count;
    comm->RXByteCtr = 0;
    comm->ReceiveIndex = 0;
    comm->TransmitIndex = 0;

    /* Initialize slave address stage */
    UCB0I2CSA = dev_addr;                    // Slave Address
    /* Initialize ISR */
    UCB0IFG &= ~(UCTXIFG + UCRXIFG);         // Clear any pending interrupts
    UCB0IE &= ~UCRXIE;                       // Disable RX interrupt
    UCB0IE |= UCTXIE;                        // Enable TX interrupt

    UCB0CTLW0 |= UCTR + UCTXSTT;             // SDA transmit mode, generte a start condition
    __enable_interrupt();

    // Block until ISR modifies the state machine variables
    while (!(comm->I2C_stMachine == IDLE_MODE));

    // Ensure STOP condition is fully clocked out to the physical bus before returning
    while (UCB0CTLW0 & UCTXSTP);

    return comm->I2C_stMachine;
}

/**
* @details
*/
I2C_Mode_t I2C_Master_IsSlaveAvailable(I2C_port_t* comm, uint8_t dev_addr) {
    // Wait for bus to be idle before initiating START condition
    while (UCB0STATW & UCBBUSY);

    comm->I2C_stMachine = SWITCH_TO_SLAVE_ACK;
    
    UCB0I2CSA = dev_addr;
    
    // Clear pending interrupts to prevent immediate false vectoring
    UCB0IFG &= ~(UCTXIFG0 | UCRXIFG0 | UCNACKIFG); 
    UCB0IE &= ~UCRXIE;                       
    UCB0IE |= UCTXIE | UCNACKIE;             // Ensure both TX and NACK interrupts are active

    UCB0CTLW0 |= UCTR | UCTXSTT;             
    __enable_interrupt();

    // Block until ISR modifies the state machine variable
    while (comm->I2C_stMachine == SWITCH_TO_SLAVE_ACK);

    // Ensure STOP condition is fully clocked out to the physical bus before returning
    while (UCB0CTLW0 & UCTXSTP);

    return comm->I2C_stMachine;
}

/**
* @details
*/
void I2C_port_init(I2C_port_t* comm) {
    //// I2C pins
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);
    
    //// eUSCIB_0 as I2C port
    UCB0CTLW0 = UCSWRST;                                     // Enable SW reset
    UCB0CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK | UCSYNC;  // I2C master mode, SMCLK
    UCB0CTLW0 &= ~UCSLA10;                                   // 7-bit Address mode
    UCB0BRW = 160;                                           // fSCL = SMCLK/160 = ~100kHz
    
    //UCB0CTLW1 = UCASTP_2;                                    // A STOP condition is generated automatically after the byte counter value 
                                                             // reached UCBxTBCNT. UCBCNTIFG is set with the byte counter reaching the threshold.
    //UCB0TBCNT = 0x07;                                       // TX 7 bytes of data
    
    UCB0CTLW0 &= ~UCSWRST;                                   // Clear SW reset, resume operation
    
    //// STATE MACHINE INIT
    UCB0IE |= UCNACKIE;                                      // enable TX-interrupt

    comm->I2C_stMachine = IDLE_MODE;
    comm->TransmitRegAddr = 0;
    memset(&(comm->ReceiveBuffer), 0, MAX_BUFFER_SIZE);
    comm->RXByteCtr = 0;
    comm->ReceiveIndex = 0;
    memset(&(comm->TransmitBuffer), 0, MAX_BUFFER_SIZE);
    comm->TXByteCtr = 0;
    comm->TransmitIndex = 0;
}    