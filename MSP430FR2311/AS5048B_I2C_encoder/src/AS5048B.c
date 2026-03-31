/**
 * @file AS5048B.c
 * @brief AS5048B Magnetic Position Sensor Driver Implementation.
 * This file contains the implementation of the driver functions for the AS5048B 
 * magnetic position sensor utilizing I2C communication. Hardware constraints and 
 * 14-bit register masking are enforced per the manufacturer's specification.
 * @reference AMS OSRAM AS5048B Datasheet, Revision 1.3.
 * @author Adrián Silva Palafox
 * @date may 31 2026
 */

#include "./include/AS5048B.h"

/* Private I2C helpers ------------------------------------------------------*/

/**
 * @internal
 * @brief Executes a hardware-abstracted I2C read transaction.
 * * @param drv Pointer to the AS5048B driver instance.
 * @param dev_id 7-bit I2C slave address of the target sensor.
 * @param reg_addr Internal register address to read from.
 * @param len Number of sequential bytes to read.
 * @return I2C_Mode_t Status of the I2C transaction (IDLE_MODE on success).
 */
static I2C_Mode_t user_i2c_read(AS5048B_Driver_t *drv,
                                uint8_t dev_id,
                                uint8_t reg_addr,
                                uint8_t len)
{
    return I2C_Master_ReadReg(drv->I2Cport, dev_id, reg_addr, len);
}

/**
 * @internal
 * @brief Executes a hardware-abstracted I2C write transaction.
 * * @param drv Pointer to the AS5048B driver instance.
 * @param dev_id 7-bit I2C slave address of the target sensor.
 * @param reg_addr Internal register address to write to.
 * @param reg_data Pointer to the payload buffer.
 * @param len Number of sequential bytes to write.
 * @return I2C_Mode_t Status of the I2C transaction (IDLE_MODE on success).
 */
static I2C_Mode_t user_i2c_write(AS5048B_Driver_t *drv,
                                 uint8_t dev_id,
                                 uint8_t reg_addr,
                                 uint8_t *reg_data,
                                 uint8_t len)
{
    return I2C_Master_WriteReg(drv->I2Cport, dev_id, reg_addr, reg_data, len);
}

/* Public API ---------------------------------------------------------------*/


I2C_Mode_t AS5048B_Init(AS5048B_Driver_t *driver, I2C_port_t *I2Cport)
{
    if (!driver || !I2Cport) return COMM_ERROR; // Null pointer verification
    driver->I2Cport = I2Cport;
    driver->device_count = 0;
    return IDLE_MODE;
}

I2C_Mode_t AS5048B_AddDevice(AS5048B_Driver_t *driver,
                             uint8_t num_encoder,
                             uint8_t dev_id)
{
    if (!driver || !(driver->I2Cport) || num_encoder >= AS5048B_MAX_DEVICES) return COMM_ERROR;
    if (driver->device_count >= AS5048B_MAX_DEVICES) return COMM_ERROR;

    driver->devices[num_encoder].dev_id = dev_id;
    driver->device_count++;

    return I2C_Master_IsSlaveAvailable(driver->I2Cport, dev_id);
}

void find_dev_id_address(AS5048B_Driver_t *driver)
{
    if (!driver || !(driver->I2Cport)) return;

    uint8_t found = 0, addr;
    for (addr = 1; addr <= MAX_I2C_ADDR && found < driver->device_count; addr++) {
        if (I2C_Master_IsSlaveAvailable(driver->I2Cport, addr) == ACK_MODE) {
            driver->devices[found++].dev_id = addr;
        }
    }
}

I2C_Mode_t AS5048B_UpdateRegisters(AS5048B_Driver_t *driver, uint8_t num_encoder)
{
    AS5048B_Sensor *sens = &driver->devices[num_encoder];
    I2C_Mode_t st = IDLE_MODE;

    st = user_i2c_read(driver, sens->dev_id, REG_PROG_CTRL, 1);
    if (st != IDLE_MODE) return st; 
    sens->registers.prog_ctrl = driver->I2Cport->ReceiveBuffer[0];

    st = user_i2c_read(driver, sens->dev_id, REG_I2C_ADDR, 3);
    if (st != IDLE_MODE) return st;
    sens->registers.i2c_slave_addr = driver->I2Cport->ReceiveBuffer[0];
    sens->registers.zero_pos_high  = driver->I2Cport->ReceiveBuffer[1];
    sens->registers.zero_pos_low   = driver->I2Cport->ReceiveBuffer[2];

    st = user_i2c_read(driver, sens->dev_id, REG_AGC, 4);
    if (st != IDLE_MODE) return st;
    sens->registers.automatic_gain_control = driver->I2Cport->ReceiveBuffer[0];
    sens->registers.diagnostics            = driver->I2Cport->ReceiveBuffer[1];
    sens->registers.magnitude_high         = driver->I2Cport->ReceiveBuffer[2];
    sens->registers.magnitude_low          = driver->I2Cport->ReceiveBuffer[3];

    return IDLE_MODE;
}

I2C_Mode_t AS5048B_SetZeroPosition(AS5048B_Driver_t *driver, uint8_t num_encoder)
{
    AS5048B_Sensor *sens = &driver->devices[num_encoder];
    uint8_t data[2] = {0, 0};
    I2C_Mode_t st;

    // 1. Clears full 14-bit OTP zero position register (Length: 2 bytes)
    st = user_i2c_write(driver, sens->dev_id, REG_ZERO_POS_HIGH, data, 2);
    if (st != IDLE_MODE) return st;

    // 2. Fetch current absolute angle
    AS5048B_GetAngle16bit(driver, num_encoder);
    
    // 3. Write previous read angle position into OTP zero position registers
    data[0] = sens->registers.angle_high;
    data[1] = sens->registers.angle_low;
    st = user_i2c_write(driver, sens->dev_id, REG_ZERO_POS_HIGH, data, 2);

    return st;
}

uint16_t AS5048B_GetAngle16bit(AS5048B_Driver_t *driver, uint8_t num_encoder)
{
    AS5048B_Sensor *sens = &driver->devices[num_encoder];

    if (user_i2c_read(driver, sens->dev_id, REG_ANGLE_HIGH, 2) != IDLE_MODE) return 0;

    sens->registers.angle_high = driver->I2Cport->ReceiveBuffer[0];
    sens->registers.angle_low = driver->I2Cport->ReceiveBuffer[1];

    // 14-bit alignment: Shifts HIGH byte to bits [13:6], applies 0x3F mask to LOW byte bits [5:0]
    return (((uint16_t)sens->registers.angle_high) << 6) | (sens->registers.angle_low & 0x3F);
}

uint16_t AS5048B_GetMagnitude(AS5048B_Driver_t *driver, uint8_t num_encoder)
{
    AS5048B_Sensor *sens = &driver->devices[num_encoder];
    
    // Targeted I2C read mitigates bus saturation
    if (user_i2c_read(driver, sens->dev_id, REG_MAGNITUDE_HIGH, 2) == IDLE_MODE) {
        sens->registers.magnitude_high = driver->I2Cport->ReceiveBuffer[0];
        sens->registers.magnitude_low  = driver->I2Cport->ReceiveBuffer[1];
    }
    return (((uint16_t)sens->registers.magnitude_high) << 6) | (sens->registers.magnitude_low & 0x3F);
}

uint8_t AS5048B_CheckDiagnostics(AS5048B_Driver_t *driver, uint8_t num_encoder)
{
    AS5048B_Sensor *sens = &driver->devices[num_encoder];
    
    // Targeted I2C read to prevent sequential fetching of unrelated AGC/Magnitude data
    if (user_i2c_read(driver, sens->dev_id, REG_DIAG, 1) == IDLE_MODE) {
        sens->registers.diagnostics = driver->I2Cport->ReceiveBuffer[0];
    }
    return sens->registers.diagnostics;
}