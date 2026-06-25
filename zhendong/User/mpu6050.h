#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f1xx_hal.h"
#include "app_types.h"
#include <stdint.h>

#define MPU6050_I2C_ADDR_LOW       (0x68U << 1)
#define MPU6050_I2C_ADDR_HIGH      (0x69U << 1)

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t address;
    float accel_lsb_per_g;
} MPU6050_Handle;

uint8_t MPU6050_Init(MPU6050_Handle *dev, I2C_HandleTypeDef *hi2c, uint8_t ad0_high);
uint8_t MPU6050_ReadWhoAmI(MPU6050_Handle *dev, uint8_t *id);
uint8_t MPU6050_ReadAccel(MPU6050_Handle *dev, AccelData *out);

#endif

