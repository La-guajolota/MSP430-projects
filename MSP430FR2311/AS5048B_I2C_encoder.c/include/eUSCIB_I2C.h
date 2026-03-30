//******************************************************************************
//   Based on the MSP430FR231x Demo - eUSCI_B0, I2C Master multiple byte TX/RX
//   by Nima Eskandari and Ryan Meredith
//   Texas Instruments Inc. February 2018
//   Description: I2C master communicates to I2C slave sending and receiving
//   3 different messages of different length. I2C master will enter LPM0 mode
//   while waiting for the messages to be sent/receiving using I2C interrupt.
//   ACLK = NA, MCLK = SMCLK = DCO 16MHz.
//******************************************************************************

#ifndef eUSCIB_I2C_H
#define eUSCIB_I2C_H

/*** Includes ***/
#include <msp430.h> 
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/*** Defines ***/
#define MAX_BUFFER_SIZE     20

/*** General I2C State Machine ***/
typedef enum I2C_ModeEnum{
    IDLE_MODE,                                 // No active port
    NACK_MODE,                                 //   
    ACK_MODE,
    TX_REG_ADDRESS_MODE,                       // Transmit device's address to communicate with
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
    SWITCH_TO_TX_MODE,
    SWITCH_TO_SLAVE_ACK,
    TIMEOUT_MODE,
    COMM_ERROR
} I2C_Mode_t;

/*** I2C-port's variables ***/
typedef struct {
    I2C_Mode_t I2C_stMachine;
    uint8_t TransmitRegAddr;                    // The Register Address/Command to use
    uint8_t ReceiveBuffer[MAX_BUFFER_SIZE];     // Buffer used to receive data
    uint8_t RXByteCtr;                          // Number of bytes left to receive
    uint8_t ReceiveIndex;                       // The index of the next byte to be received in ReceiveBuffer
    uint8_t TransmitBuffer[MAX_BUFFER_SIZE];    // Buffer used to transmit data
    uint8_t TXByteCtr;                          // Number of bytes left to transfer
    uint8_t TransmitIndex;                      // The index of the next byte to be transmitted in TransmitBuffer   
} I2C_port_t;

/*** Low-level I2C communication functions ***/
/**
* @brief For slave device with dev_addr, writes the data specified in *reg_data
* @param comm: 
* @param dev_addr: The slave device address.
* @param reg_addr: The register or command to send to the slave.
* @param reg_data: The buffer to write
* @param count: The length of *reg_data
* @return
*/
I2C_Mode_t I2C_Master_WriteReg(I2C_port_t* comm, uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);

/**
* @brief For slave device with dev_addr, read the data specified in slaves reg_addr.
*        The received data is available in ReceiveBuffer
* @param comm: 
* @param dev_addr: The slave device address.
* @param reg_addr: The register or command to send to the slave.
* @param count: The length of data to read.
* @return
*/
I2C_Mode_t I2C_Master_ReadReg(I2C_port_t* comm, uint8_t dev_addr, uint8_t reg_addr, uint8_t count);

/**
* @brief 
* @param comm: 
* @return
*/
I2C_Mode_t I2C_Master_IsSlaveAvailable(I2C_port_t* comm, uint8_t dev_addr);

/**
* @brief
* @param
* @return
*/
void I2C_port_t_init(I2C_port_t*);
#endif /* eUSCIB_I2C_H */