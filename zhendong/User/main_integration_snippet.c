/*
 * This file is not meant to be compiled directly.
 * Copy the snippets into CubeMX generated files.
 */

/* main.c */
#include "app_vibration.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_IWDG_Init();

    AppVibration_Start();
    vTaskStartScheduler();

    while (1) {
    }
}

/*
 * If CubeMX uses CMSIS-RTOS and generates MX_FREERTOS_Init(),
 * place AppVibration_Start() inside MX_FREERTOS_Init().
 */
