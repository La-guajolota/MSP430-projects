#include "./include/eUSCIB_I2C.h"

I2C_Mode_t I2C_Master_ReadReg(I2C_port_t* comm, uint8_t dev_addr, uint8_t reg_addr, uint8_t count) {
    while (UCB0STATW & UCBBUSY);
    
    comm->I2C_stMachine = TX_REG_ADDRESS_MODE;
    comm->TransmitRegAddr = reg_addr;
    comm->RXByteCtr = count;
    comm->TXByteCtr = 0;
    comm->ReceiveIndex = 0;
    comm->TransmitIndex = 0;

    UCB0I2CSA = dev_addr;                    
    UCB0IFG &= ~(UCTXIFG + UCRXIFG);         
    UCB0IE &= ~UCRXIE;                       
    UCB0IE |= UCTXIE;                        

    UCB0CTLW0 |= UCTR | UCTXSTT;             
    __enable_interrupt();

    // Evaluates for both success and NACK failure
    while (comm->I2C_stMachine != IDLE_MODE && comm->I2C_stMachine != NACK_MODE);

    while (UCB0CTLW0 & UCTXSTP);

    return comm->I2C_stMachine;
}

I2C_Mode_t I2C_Master_WriteReg(I2C_port_t* comm, uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count) {
    while (UCB0STATW & UCBBUSY);
    
    comm->I2C_stMachine = TX_REG_ADDRESS_MODE;
    comm->TransmitRegAddr = reg_addr;

    // Corrected pointer decay strict aliasing violation
    memcpy(comm->TransmitBuffer, reg_data, count);

    comm->TXByteCtr = count;
    comm->RXByteCtr = 0;
    comm->ReceiveIndex = 0;
    comm->TransmitIndex = 0;

    UCB0I2CSA = dev_addr;                    
    UCB0IFG &= ~(UCTXIFG + UCRXIFG);         
    UCB0IE &= ~UCRXIE;                       
    UCB0IE |= UCTXIE;                        

    UCB0CTLW0 |= UCTR | UCTXSTT;             
    __enable_interrupt();

    // Evaluates for both success and NACK failure
    while (comm->I2C_stMachine != IDLE_MODE && comm->I2C_stMachine != NACK_MODE);

    while (UCB0CTLW0 & UCTXSTP);

    return comm->I2C_stMachine;
}

I2C_Mode_t I2C_Master_IsSlaveAvailable(I2C_port_t* comm, uint8_t dev_addr) {
    while (UCB0STATW & UCBBUSY);

    comm->I2C_stMachine = SWITCH_TO_SLAVE_ACK;
    UCB0I2CSA = dev_addr;
    
    UCB0IFG &= ~(UCTXIFG0 | UCRXIFG0 | UCNACKIFG); 
    UCB0IE &= ~UCRXIE;                       
    UCB0IE |= UCTXIE | UCNACKIE;             

    UCB0CTLW0 |= UCTR | UCTXSTT;             
    __enable_interrupt();

    while (comm->I2C_stMachine == SWITCH_TO_SLAVE_ACK);
    while (UCB0CTLW0 & UCTXSTP);

    return comm->I2C_stMachine;
}

void I2C_port_init(I2C_port_t* comm) {
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);
    
    UCB0CTLW0 = UCSWRST;                                     
    UCB0CTLW0 |= UCMODE_3 | UCMST | UCSSEL__SMCLK | UCSYNC;  
    UCB0CTLW0 &= ~UCSLA10;                                   
    UCB0BRW = 160;                                           
    UCB0CTLW0 &= ~UCSWRST;                                   
    
    UCB0IE |= UCNACKIE;                                      

    comm->I2C_stMachine = IDLE_MODE;
    comm->TransmitRegAddr = 0;
    
    // Corrected pointer decay strict aliasing violation
    memset(comm->ReceiveBuffer, 0, MAX_BUFFER_SIZE);
    memset(comm->TransmitBuffer, 0, MAX_BUFFER_SIZE);
    
    comm->RXByteCtr = 0;
    comm->ReceiveIndex = 0;
    comm->TXByteCtr = 0;
    comm->TransmitIndex = 0;
}