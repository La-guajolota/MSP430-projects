/**
 * @file eUSCIB_I2C.h
 * @brief Hardware Abstraction Layer for eUSCI_B I2C Master communication.
 * * Implements interrupt-driven state machines for non-blocking I2C hardware 
 * transitions, supporting repeated starts and slave availability polling.
 * @author Adrián Silva Palafox
 * @date may 31 2026
 */
#ifndef eUSCIB_I2C_H
#define eUSCIB_I2C_H

#include <msp430.h> 
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MAX_BUFFER_SIZE 20

/**
 * @brief Enumeration of internal I2C hardware state machine states.
 */
typedef enum I2C_ModeEnum {
    IDLE_MODE,
    NACK_MODE,
    ACK_MODE,
    TX_REG_ADDRESS_MODE,
    RX_REG_ADDRESS_MODE,
    TX_DATA_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
    SWITCH_TO_TX_MODE,
    SWITCH_TO_SLAVE_ACK,
    TIMEOUT_MODE,
    COMM_ERROR
} I2C_Mode_t;

/**
 * @brief Structure encapsulating I2C port state, buffers, and transaction metrics.
 */
typedef struct {
    volatile I2C_Mode_t I2C_stMachine;          // Volatile required for ISR modification visibility
    uint8_t TransmitRegAddr;                    
    uint8_t ReceiveBuffer[MAX_BUFFER_SIZE];     
    volatile uint8_t RXByteCtr;                 
    volatile uint8_t ReceiveIndex;              
    uint8_t TransmitBuffer[MAX_BUFFER_SIZE];    
    volatile uint8_t TXByteCtr;                 
    volatile uint8_t TransmitIndex;             
} I2C_port_t;

/**
 * @brief Executes a multi-byte I2C Master Write sequence.
 * * @param comm Pointer to the I2C port struct mapping the hardware state.
 * @param dev_addr 7-bit destination slave address.
 * @param reg_addr Target internal register address on the slave device.
 * @param reg_data Pointer to the data payload buffer to be transmitted.
 * @param count Number of bytes to transmit from the payload buffer.
 * @return I2C_Mode_t Final state of the transaction (IDLE_MODE on success, NACK_MODE on failure).
 */
I2C_Mode_t I2C_Master_WriteReg(I2C_port_t* comm, uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint8_t count);

/**
 * @brief Executes a multi-byte I2C Master Read sequence using a repeated start.
 * * @param comm Pointer to the I2C port struct mapping the hardware state.
 * @param dev_addr 7-bit destination slave address.
 * @param reg_addr Target internal register address on the slave device to read from.
 * @param count Number of bytes to read into the ReceiveBuffer.
 * @return I2C_Mode_t Final state of the transaction.
 */
I2C_Mode_t I2C_Master_ReadReg(I2C_port_t* comm, uint8_t dev_addr, uint8_t reg_addr, uint8_t count);

/**
 * @brief Probes the I2C bus to determine if a specific slave address acknowledges.
 * * @param comm Pointer to the I2C port struct mapping the hardware state.
 * @param dev_addr 7-bit target slave address to probe.
 * @return I2C_Mode_t ACK_MODE if the device responds, NACK_MODE if absent.
 */
I2C_Mode_t I2C_Master_IsSlaveAvailable(I2C_port_t* comm, uint8_t dev_addr);

/**
 * @brief Initializes the eUSCI_B hardware for I2C Master mode at 100 kHz.
 * * @param comm Pointer to the I2C port struct to be zero-initialized.
 */
void I2C_port_init(I2C_port_t* comm);

#endif /* eUSCIB_I2C_H */