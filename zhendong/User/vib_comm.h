#ifndef VIB_COMM_H
#define VIB_COMM_H

#include "stm32f1xx_hal.h"
#include "app_types.h"

void VibComm_SendJson(UART_HandleTypeDef *huart, const VibResult *result);

#endif

