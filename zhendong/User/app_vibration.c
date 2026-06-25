#include "app_vibration.h"
#include "app_config.h"
#include "app_types.h"
#include "mpu6050.h"
#include "vib_ring_buffer.h"
#include "vib_feature.h"
#include "vib_detect.h"
#include "vib_comm.h"

#include "main.h"
#if VIB_ENABLE_IWDG
#include "iwdg.h"
#endif
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
#if VIB_ENABLE_IWDG
extern IWDG_HandleTypeDef hiwdg;
#endif

typedef enum {
    VIB_WD_SENSOR = 0,
    VIB_WD_PROCESS,
    VIB_WD_DETECT,
    VIB_WD_COMM,
    VIB_WD_COUNT
} VibWatchdogChannel;

static MPU6050_Handle g_mpu;
static VibRingBuffer g_ring;
static float g_ring_storage[VIB_WINDOW_SIZE];
static float g_process_window[VIB_WINDOW_SIZE];
static QueueHandle_t g_feature_queue;
static QueueHandle_t g_result_queue;
static TaskHandle_t g_process_task_handle;
static uint32_t g_seq;
static uint32_t g_dropped_windows;
static volatile TickType_t g_task_heartbeat[VIB_WD_COUNT];

static void SensorTask(void *argument);
static void ProcessTask(void *argument);
static void DetectTask(void *argument);
static void CommTask(void *argument);
#if VIB_ENABLE_IWDG
static void WatchdogTask(void *argument);
#endif
static void WatchdogKick(VibWatchdogChannel channel);
static void DebugUart(const char *msg);
#if VIB_ENABLE_IWDG
static uint8_t WatchdogFresh(TickType_t now, VibWatchdogChannel channel, uint32_t timeout_ms);
static uint8_t WatchdogAllTasksHealthy(TickType_t now);
#endif

void AppVibration_Start(void)
{
    BaseType_t ok;

    DebugUart("APP_START\r\n");

    VibRing_Init(&g_ring, g_ring_storage, VIB_WINDOW_SIZE);
    g_feature_queue = xQueueCreate(VIB_FEATURE_QUEUE_LEN, sizeof(VibFeature));
    g_result_queue = xQueueCreate(VIB_RESULT_QUEUE_LEN, sizeof(VibResult));

    if (g_feature_queue == NULL || g_result_queue == NULL) {
        DebugUart("ERR_QUEUE_CREATE\r\n");
        Error_Handler();
    }

    ok = xTaskCreate(SensorTask, "vib_sensor", VIB_SENSOR_TASK_STACK, NULL,
                     tskIDLE_PRIORITY + VIB_SENSOR_TASK_PRIO_OFFSET, NULL);
    if (ok != pdPASS) {
        DebugUart("ERR_SENSOR_TASK\r\n");
        Error_Handler();
    }

    ok = xTaskCreate(ProcessTask, "vib_process", VIB_PROCESS_TASK_STACK, NULL,
                     tskIDLE_PRIORITY + VIB_PROCESS_TASK_PRIO_OFFSET, &g_process_task_handle);
    if (ok != pdPASS) {
        DebugUart("ERR_PROCESS_TASK\r\n");
        Error_Handler();
    }

    ok = xTaskCreate(DetectTask, "vib_detect", VIB_DETECT_TASK_STACK, NULL,
                     tskIDLE_PRIORITY + VIB_DETECT_TASK_PRIO_OFFSET, NULL);
    if (ok != pdPASS) {
        DebugUart("ERR_DETECT_TASK\r\n");
        Error_Handler();
    }

    ok = xTaskCreate(CommTask, "vib_comm", VIB_COMM_TASK_STACK, NULL,
                     tskIDLE_PRIORITY + VIB_COMM_TASK_PRIO_OFFSET, NULL);
    if (ok != pdPASS) {
        DebugUart("ERR_COMM_TASK\r\n");
        Error_Handler();
    }
#if VIB_ENABLE_IWDG
    ok = xTaskCreate(WatchdogTask, "vib_wdg", VIB_WATCHDOG_TASK_STACK, NULL,
                     tskIDLE_PRIORITY + VIB_WATCHDOG_TASK_PRIO_OFFSET, NULL);
    if (ok != pdPASS) {
        DebugUart("ERR_WDG_TASK\r\n");
        Error_Handler();
    }
#endif

    DebugUart("APP_TASKS_OK\r\n");
}

static void SensorTask(void *argument)
{
    TickType_t last_wake;
    const TickType_t period = pdMS_TO_TICKS(1000U / VIB_SAMPLE_HZ);
    AccelData acc;
    uint16_t hop_count = 0U;

    (void)argument;
    last_wake = xTaskGetTickCount();
    WatchdogKick(VIB_WD_SENSOR);

    if (!MPU6050_Init(&g_mpu, &hi2c1, 0U)) {
        VibResult err = {0};
        err.state = VIB_STATE_SENSOR_ERROR;
        xQueueSend(g_result_queue, &err, 0U);
        DebugUart("ERR_MPU6050_INIT\r\n");
        vTaskDelete(NULL);
    }

    DebugUart("MPU6050_OK\r\n");

    for (;;) {
        if (MPU6050_ReadAccel(&g_mpu, &acc)) {
            float mag = VibFeature_VectorMagnitude(&acc);

            taskENTER_CRITICAL();
            VibRing_Push(&g_ring, mag);
            taskEXIT_CRITICAL();

            if (VibRing_IsFull(&g_ring)) {
                hop_count++;
                if (hop_count >= VIB_HOP_SIZE) {
                    hop_count = 0U;
                    xTaskNotifyGive(g_process_task_handle);
                }
            }
        }
        WatchdogKick(VIB_WD_SENSOR);
        vTaskDelayUntil(&last_wake, period);
    }
}

static void ProcessTask(void *argument)
{
    VibFeature feature;

    (void)argument;
    WatchdogKick(VIB_WD_PROCESS);

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        taskENTER_CRITICAL();
        VibRing_CopyLatest(&g_ring, g_process_window, VIB_WINDOW_SIZE);
        taskEXIT_CRITICAL();

        VibFeature_Calc(g_process_window, VIB_WINDOW_SIZE, &feature);
        g_seq++;

        if (xQueueSend(g_feature_queue, &feature, 0U) != pdPASS) {
            g_dropped_windows++;
        }
        WatchdogKick(VIB_WD_PROCESS);
    }
}

static void DetectTask(void *argument)
{
    VibDetector detector;
    VibFeature feature;
    VibResult result;

    (void)argument;
    VibDetect_Init(&detector);
    WatchdogKick(VIB_WD_DETECT);

    for (;;) {
        if (xQueueReceive(g_feature_queue, &feature, portMAX_DELAY) == pdPASS) {
            result = VibDetect_Update(&detector, &feature, g_seq, g_dropped_windows);
            if (xQueueSend(g_result_queue, &result, 0U) != pdPASS) {
                g_dropped_windows++;
            }
            WatchdogKick(VIB_WD_DETECT);
        }
    }
}

static void CommTask(void *argument)
{
    VibResult result;

    (void)argument;
    WatchdogKick(VIB_WD_COMM);

    for (;;) {
        if (xQueueReceive(g_result_queue, &result, portMAX_DELAY) == pdPASS) {
            VibComm_SendJson(&huart1, &result);
            WatchdogKick(VIB_WD_COMM);
        }
    }
}

#if VIB_ENABLE_IWDG
static void WatchdogTask(void *argument)
{
    (void)argument;

    for (;;) {
        TickType_t now = xTaskGetTickCount();
        if (WatchdogAllTasksHealthy(now)) {
            HAL_IWDG_Refresh(&hiwdg);
        }
        vTaskDelay(pdMS_TO_TICKS(VIB_WATCHDOG_PERIOD_MS));
    }
}
#endif

static void WatchdogKick(VibWatchdogChannel channel)
{
    g_task_heartbeat[channel] = xTaskGetTickCount();
}

static void DebugUart(const char *msg)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 100U);
}

#if VIB_ENABLE_IWDG
static uint8_t WatchdogFresh(TickType_t now, VibWatchdogChannel channel, uint32_t timeout_ms)
{
    TickType_t age = now - g_task_heartbeat[channel];
    return age <= pdMS_TO_TICKS(timeout_ms);
}

static uint8_t WatchdogAllTasksHealthy(TickType_t now)
{
    return WatchdogFresh(now, VIB_WD_SENSOR, VIB_WATCHDOG_SENSOR_TIMEOUT_MS) &&
           WatchdogFresh(now, VIB_WD_PROCESS, VIB_WATCHDOG_PROCESS_TIMEOUT_MS) &&
           WatchdogFresh(now, VIB_WD_DETECT, VIB_WATCHDOG_DETECT_TIMEOUT_MS) &&
           WatchdogFresh(now, VIB_WD_COMM, VIB_WATCHDOG_COMM_TIMEOUT_MS);
}
#endif
