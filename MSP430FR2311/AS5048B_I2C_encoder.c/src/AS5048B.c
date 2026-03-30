// /**
//  * AS5048B Magnetic Position Sensor Driver Implementation
//  *
//  * This file contains the implementation of the driver functions
//  * for the AS5048B magnetic position sensor using I2C communication.
//  */

#include "./include/AS5048B.h"

// /* Private I2C helpers ------------------------------------------------------*/
static I2C_Mode_t user_i2c_read(AS5048B_Driver_t *drv,
                                      uint8_t dev_id,
                                      uint8_t reg_addr,
                                      uint8_t len)
{
    return I2C_Master_ReadReg(drv->I2Cport, dev_id, reg_addr, len);
}

static I2C_Mode_t user_i2c_write(AS5048B_Driver_t *drv,
                                       uint8_t dev_id,
                                       uint8_t reg_addr,
                                       uint8_t *reg_data,
                                       uint8_t len)
{
    return I2C_Master_WriteReg(drv->I2Cport, dev_id, reg_addr, reg_data, len);
}

// /* Public API ---------------------------------------------------------------*/
I2C_Mode_t AS5048B_Init(AS5048B_Driver_t *driver,
                               I2C_port_t *I2Cport)
{
    // to-do verify null pointer
    driver->I2Cport = I2Cport;
    driver->device_count = 0;
    return IDLE_MODE;
}

I2C_Mode_t AS5048B_AddDevice(AS5048B_Driver_t *driver,
                                    uint8_t num_encoder,
                                    uint8_t dev_id)
{
    // to-do verify null pointer
    if (num_encoder >= AS5048B_MAX_DEVICES) return COMM_ERROR;
    if (driver->device_count >= AS5048B_MAX_DEVICES) return COMM_ERROR;

    driver->devices[num_encoder].dev_id = dev_id; // Use 7-bit address
    driver->device_count++;

    // Verify if there's connection to device
    return I2C_Master_IsSlaveAvailable(driver->I2Cport, dev_id);
}

void find_dev_id_address(AS5048B_Driver_t *driver)
{
    uint8_t found = 0, addr;
    for (addr = 1; addr <= MAX_I2C_ADDR && found < driver->device_count; addr++) {
        if (I2C_Master_IsSlaveAvailable(driver->I2Cport, addr) == ACK_MODE) {
            driver->devices[found++].dev_id = addr;
        }
    }
}

I2C_Mode_t AS5048B_UpdateRegisters(AS5048B_Driver_t *driver,
										  uint8_t num_encoder)
{
    AS5048B_Sensor *sens = &driver->devices[num_encoder];
    I2C_Mode_t st;

    st |= user_i2c_read(driver, sens->dev_id, REG_PROG_CTRL, 1);
    st |= user_i2c_read(driver, sens->dev_id, REG_I2C_ADDR, 3);    // read from REG_I2C_ADDR to REG_ZERO_POS_LOW
    st |= user_i2c_read(driver, sens->dev_id, REG_AGC, 4);         // read from REG_AGC to REG_MAGNITUDE_LOW

    sens->registers.prog_ctrl              = driver->I2Cport->ReceiveBuffer[0];
    sens->registers.i2c_slave_addr         = driver->I2Cport->ReceiveBuffer[1];
    sens->registers.zero_pos_high          = driver->I2Cport->ReceiveBuffer[2];
    sens->registers.zero_pos_low           = driver->I2Cport->ReceiveBuffer[3];
    sens->registers.automatic_gain_control = driver->I2Cport->ReceiveBuffer[4];
    sens->registers.diagnostics            = driver->I2Cport->ReceiveBuffer[5];
    sens->registers.magnitude_high         = driver->I2Cport->ReceiveBuffer[6];
    sens->registers.magnitude_low          = driver->I2Cport->ReceiveBuffer[7];
    return st;
}

// I2C_Mode_t AS5048B_SetZeroPosition(AS5048B_Driver_t *driver,
//                                           uint8_t num_encoder)
// {
//     AS5048B_Sensor *sens = &driver->devices[num_encoder];
//     uint8_t data[2] = {0,0};
//     I2C_Mode_t st;

//     /*
//      * Programming Sequence with Verification: To program the zero position is needed to perform following sequence:
// 	 * 1. Write 0 into OTP zero position register to clear
// 	 * 2. Read angle information
// 	 * 3. Write previous read angle position into OTP zero position register
// 	 * Now the zero position is set.
// 	 * !!!!!! If you want to burn it to the OTP register send: !!!!!
// 	 * 4. Set the Programming Enable bit in the OTP control register
// 	 * 5. Set the Burn bit to start the automatic programming procedure
// 	 * 6. Read angle information (equals to 0)
//      * 7. Set the Verify bit to load the OTP data again into the internal registers
// 	 * 8. Read angle information (equals to 0)
//      */
//     // 1.-
//     st = user_i2c_write(driver, sens->dev_id, REG_ZERO_POS_HIGH, data, 1);
//     // 2.-
//     AS5048B_GetAngleDegrees(driver, sens->dev_id);
//     data[0] = sens->registers.angle_high;
//     data[1] = sens->registers.angle_low;
//     // 3-
//     st = user_i2c_write(driver, sens->dev_id, REG_ZERO_POS_HIGH, data, 2);

//     return st;
// }

// float AS5048B_GetAngleDegrees(AS5048B_Driver_t *driver,
//                               uint8_t num_encoder)
// {
//     AS5048B_Sensor *sens = &driver->devices[num_encoder];
//     uint8_t data[2];

//     if (user_i2c_read(driver, sens->dev_id, REG_ANGLE_HIGH, data, 2)) return -1.0f; //error

//     /* Store raw */
//     sens->registers.angle_high = data[0];
//     sens->registers.angle_low = data[1];

//     /* Compute it to degrees*/
//     uint8_t raw = ((uint8_t)data[1] << 6) | (data[0] >> 2);
//     return raw * 360.0f / 16384.0f;
// }

// float AS5048B_GetAngleRadians(AS5048B_Driver_t *driver,
//                               uint8_t num_encoder)
// {
//     return AS5048B_GetAngleDegrees(driver, num_encoder) * 3.14159265359f / 180.0f;
// }

// uint8_t AS5048B_GetMagnitude(AS5048B_Driver_t *driver,
//                               uint8_t num_encoder)
// {
//     AS5048B_UpdateRegisters(driver, num_encoder);
//     AS5048B_Sensor *sens = &driver->devices[num_encoder];
//     return (sens->registers.magnitude_high << 6) | sens->registers.magnitude_low;
// }

// uint8_t AS5048B_CheckDiagnostics(AS5048B_Driver_t *driver,
//                                   uint8_t num_encoder)
// {
//     AS5048B_UpdateRegisters(driver, num_encoder);
//     return driver->devices[num_encoder].registers.diagnostics;
// }
