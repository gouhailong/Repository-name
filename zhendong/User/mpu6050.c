#include "mpu6050.h"

#define MPU6050_REG_SMPLRT_DIV     0x19U
#define MPU6050_REG_CONFIG         0x1AU
#define MPU6050_REG_ACCEL_CONFIG   0x1CU
#define MPU6050_REG_ACCEL_XOUT_H   0x3BU
#define MPU6050_REG_PWR_MGMT_1     0x6BU
#define MPU6050_REG_WHO_AM_I       0x75U

static uint8_t mpu_write_u8(MPU6050_Handle *dev, uint8_t reg, uint8_t data)
{
    return HAL_I2C_Mem_Write(dev->hi2c, dev->address, reg, I2C_MEMADD_SIZE_8BIT,
                             &data, 1U, 100U) == HAL_OK;
}

static uint8_t mpu_read(MPU6050_Handle *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(dev->hi2c, dev->address, reg, I2C_MEMADD_SIZE_8BIT,
                            buf, len, 100U) == HAL_OK;
}

uint8_t MPU6050_ReadWhoAmI(MPU6050_Handle *dev, uint8_t *id)
{
    return mpu_read(dev, MPU6050_REG_WHO_AM_I, id, 1U);
}

uint8_t MPU6050_Init(MPU6050_Handle *dev, I2C_HandleTypeDef *hi2c, uint8_t ad0_high)
{
    uint8_t id = 0;

    dev->hi2c = hi2c;
    dev->address = ad0_high ? MPU6050_I2C_ADDR_HIGH : MPU6050_I2C_ADDR_LOW;
    dev->accel_lsb_per_g = 16384.0f;

    if (!MPU6050_ReadWhoAmI(dev, &id)) {
        return 0;
    }
    if (id != 0x68U && id != 0x69U) {
        return 0;
    }

    if (!mpu_write_u8(dev, MPU6050_REG_PWR_MGMT_1, 0x00U)) return 0;
    HAL_Delay(50U);

    /*
     * CONFIG[2:0] = 3: digital low pass filter about 44 Hz for accel.
     * This makes the demo more stable for low-frequency vibration.
     */
    if (!mpu_write_u8(dev, MPU6050_REG_CONFIG, 0x03U)) return 0;

    /*
     * Sample rate = gyro output rate / (1 + SMPLRT_DIV).
     * With DLPF enabled, gyro output rate is 1 kHz. 0x04 -> 200 Hz.
     * We still use task timing as the top-level sampling schedule.
     */
    if (!mpu_write_u8(dev, MPU6050_REG_SMPLRT_DIV, 0x04U)) return 0;

    /* ACCEL_CONFIG = 0: +/-2g, sensitivity 16384 LSB/g. */
    if (!mpu_write_u8(dev, MPU6050_REG_ACCEL_CONFIG, 0x00U)) return 0;

    return 1;
}

uint8_t MPU6050_ReadAccel(MPU6050_Handle *dev, AccelData *out)
{
    uint8_t buf[6];
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;

    if (!mpu_read(dev, MPU6050_REG_ACCEL_XOUT_H, buf, 6U)) {
        return 0;
    }

    raw_x = (int16_t)((uint16_t)buf[0] << 8 | buf[1]);
    raw_y = (int16_t)((uint16_t)buf[2] << 8 | buf[3]);
    raw_z = (int16_t)((uint16_t)buf[4] << 8 | buf[5]);

    out->ax = (float)raw_x / dev->accel_lsb_per_g;
    out->ay = (float)raw_y / dev->accel_lsb_per_g;
    out->az = (float)raw_z / dev->accel_lsb_per_g;
    return 1;
}

