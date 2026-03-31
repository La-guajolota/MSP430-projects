/**
 * @file AS5048B.h
 * AS5048B Magnetic Position Sensor Driver
 *
 * @brief: The host MCU (master) initiates all data transfers.
 * The 7-bit slave device address depends on the state of
 * OTP I²C register 21 (0x15) bits 0-4 plus 2 I²C address
 * selection pins (3 and 4).
 * @author Adrián Silva Palafox
 * @date may 31 2026
 */

#ifndef AS5048B_H
#define AS5048B_H

/* Includes ------------------------------------------------------------------*/
#include <math.h>
#include "./eUSCIB_I2C.h"

/* I2C Device Limits --------------------------------------------------------*/
#define AS5048B_MAX_DEVICES     1     /* How many of this IC are in the bus??*/
#define MAX_I2C_ADDR           127
#define AS5048B_DEFAULT_ADDR   0x41U  /* Default 7-bit I2C address */

/* AS5048B Register Addresses ----------------------------------------------*/
enum {
    REG_PROG_CTRL        = 0x03,
    REG_I2C_ADDR         = 0x15,
    REG_ZERO_POS_HIGH    = 0x16,
    REG_ZERO_POS_LOW     = 0x17,
    REG_AGC              = 0xFA,
    REG_DIAG             = 0xFB,
    REG_MAGNITUDE_HIGH   = 0xFC,
    REG_MAGNITUDE_LOW    = 0xFD,
    REG_ANGLE_HIGH       = 0xFE,
    REG_ANGLE_LOW        = 0xFF
};

/* AS5048B Register Map -----------------------------------------------------*/
typedef struct {
    /* Measurement Output */
    uint8_t angle_low        : 6;   /* 6 LSBs of angle */
    uint8_t                  : 2;   /* padding */
    uint8_t angle_high;             /* 8 MSBs of angle */

    uint8_t magnitude_low    : 6;  /* 6 LSBs of magnitude */
    uint8_t                  : 2;  /* padding */
    uint8_t magnitude_high;        /* 8 MSBs of magnitude */

    uint8_t diagnostics      : 4;  /* diagnostic flags */
    uint8_t automatic_gain_control;/* AGC value */

    /* Customer Settings */
    uint8_t zero_pos_low     : 6;  /* 6 LSBs of zero position */
    uint8_t                  : 2;  /* padding */
    uint8_t zero_pos_high;         /* 8 MSBs of zero position */
    uint8_t i2c_slave_addr   : 5;  /* programmable I2C address */
    uint8_t                  : 3;  /* padding */

    /* OTP Programming */
    uint8_t prog_ctrl        : 7;  /* programming control bits */
    uint8_t                  : 1;  /* padding */
} AS5048B_Registers;

/* Sensor Descriptor --------------------------------------------------------*/
typedef struct {
    AS5048B_Registers registers;   /**< Register cache */
    uint8_t dev_id;                /**< 7-bit I2C address */
} AS5048B_Sensor;

/* Driver Control Structure ------------------------------------------------*/
typedef struct {
    I2C_port_t *I2Cport;                                 /**< I2C handle */
    AS5048B_Sensor     devices[AS5048B_MAX_DEVICES];     /**< Sensor's address array */
    uint8_t            device_count;                     /**< Found devices */
} AS5048B_Driver_t;

/* Public API ----------------------------------------------------------------*/

/**
 * @brief Initializes the AS5048B driver instance and binds the I2C hardware port.
 * * @param driver Pointer to the AS5048B driver structure to be initialized.
 * @param I2Cport Pointer to the initialized I2C hardware abstraction port.
 * @return I2C_Mode_t COMM_ERROR if null pointers are passed; otherwise IDLE_MODE.
 */
I2C_Mode_t AS5048B_Init(AS5048B_Driver_t *driver,
                               I2C_port_t *I2Cport);

/**
 * @brief Registers a specific AS5048B sensor to the driver instance.
 * * @param driver Pointer to the AS5048B driver instance.
 * @param num_encoder Logical index assigned to the sensor instance (0 to AS5048B_MAX_DEVICES-1).
 * @param dev_id 7-bit I2C hardware address of the sensor.
 * @return I2C_Mode_t ACK_MODE if the device acknowledges the bus; otherwise COMM_ERROR or NACK_MODE.
 * * @warning Fails if maximum device count is exceeded or hardware port is uninitialized.
 */
I2C_Mode_t AS5048B_AddDevice(AS5048B_Driver_t *driver,
                                    uint8_t num_encoder,
                                    uint8_t dev_id);

/**
 * @brief Scans the I2C bus to automatically detect and register AS5048B sensors.
 * * Iterates through the 7-bit address space (0x01 to MAX_I2C_ADDR) and populates 
 * the driver struct with acknowledging devices until AS5048B_MAX_DEVICES is reached.
 * * @param driver Pointer to the AS5048B driver instance.
 */
void find_dev_id_address(AS5048B_Driver_t *driver);

/**
 * @brief Calibrates the sensor by writing the current mechanical angle to the Zero Position registers.
 * * Executes the sequential register clearing and writing procedure defined in the 
 * AS5048B datasheet section 7.2.1. This does not permanently burn the OTP memory.
 * * @param driver Pointer to the AS5048B driver instance.
 * @param num_encoder Logical index of the target sensor.
 * @return I2C_Mode_t IDLE_MODE upon successful calibration execution.
 */
I2C_Mode_t AS5048B_SetZeroPosition(AS5048B_Driver_t *driver,
                            			  uint8_t num_encoder);

/**
 * @brief Sequentially updates all non-volatile and diagnostic registers from the sensor.
 * * Consumes approximately 1.2ms of I2C bus time at 100kHz clock speed.
 * * @param driver Pointer to the AS5048B driver instance.
 * @param num_encoder Logical index of the target sensor.
 * @return I2C_Mode_t IDLE_MODE upon successful sequential read; otherwise the specific error state.
 */
I2C_Mode_t AS5048B_UpdateRegisters(AS5048B_Driver_t *driver,
									      uint8_t num_encoder);

/**
 * @brief Retrieves the raw 14-bit absolute angular position.
 * * Reconstructs the 14-bit value from two sequential 8-bit registers, 
 * enforcing a 0x3F mask on the low byte to drop diagnostic flags.
 * * @param driver Pointer to the AS5048B driver instance.
 * @param num_encoder Logical index of the target sensor.
 * @return uint16_t Raw 14-bit angular value (0 to 16383). Returns 0 on I2C failure.
 */
uint16_t AS5048B_GetAngle16bit(AS5048B_Driver_t *driver,
                             uint8_t num_encoder);

/**
 * @brief Retrieves the 14-bit magnetic field magnitude.
 * * @param driver Pointer to the AS5048B driver instance.
 * @param num_encoder Logical index of the target sensor.
 * @return uint16_t 14-bit magnitude value. High values indicate a strong magnetic field.
 */
uint16_t AS5048B_GetMagnitude(AS5048B_Driver_t *driver,
                              uint8_t num_encoder);

/**
 * @brief Retrieves the 8-bit diagnostic register evaluating magnetic field strength parameters.
 * * @param driver Pointer to the AS5048B driver instance.
 * @param num_encoder Logical index of the target sensor.
 * @return uint8_t Diagnostic flags (OCF, COF, COMP low/high).
 */
uint8_t AS5048B_CheckDiagnostics(AS5048B_Driver_t *driver,
                                  uint8_t num_encoder);

#endif /* AS5048B_H */