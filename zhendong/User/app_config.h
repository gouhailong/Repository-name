#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/*
 * F103C8T6 RAM is small, so start with a 128-point window.
 * If you use F407/F103RCT6, 256 points gives better frequency resolution.
 */
#define VIB_SAMPLE_HZ              200U
#define VIB_WINDOW_SIZE            128U
#define VIB_HOP_SIZE               64U

#define VIB_FEATURE_QUEUE_LEN      4U
#define VIB_RESULT_QUEUE_LEN       4U

#define VIB_SENSOR_TASK_STACK      256U
#define VIB_PROCESS_TASK_STACK     384U
#define VIB_DETECT_TASK_STACK      256U
#define VIB_COMM_TASK_STACK        384U
#define VIB_WATCHDOG_TASK_STACK    192U

#define VIB_SENSOR_TASK_PRIO_OFFSET    4U
#define VIB_PROCESS_TASK_PRIO_OFFSET   3U
#define VIB_DETECT_TASK_PRIO_OFFSET    2U
#define VIB_COMM_TASK_PRIO_OFFSET      1U
#define VIB_WATCHDOG_TASK_PRIO_OFFSET  1U

#define VIB_LEARN_WINDOWS          20U
#define VIB_WARNING_SCORE          60U
#define VIB_ALARM_SCORE            75U
#define VIB_ALARM_HOLD_WINDOWS     3U
#define VIB_RECOVER_HOLD_WINDOWS   5U

/*
 * Enable IWDG in CubeMX before setting this to 1.
 * F103 LSI is about 40 kHz. Prescaler 64 and Reload 1249 gives about 2 s.
 */
#define VIB_ENABLE_IWDG            0U
#define VIB_WATCHDOG_PERIOD_MS     500U
#define VIB_WATCHDOG_SENSOR_TIMEOUT_MS  1000U
#define VIB_WATCHDOG_PROCESS_TIMEOUT_MS 2500U
#define VIB_WATCHDOG_DETECT_TIMEOUT_MS  2500U
#define VIB_WATCHDOG_COMM_TIMEOUT_MS    3000U

#endif
