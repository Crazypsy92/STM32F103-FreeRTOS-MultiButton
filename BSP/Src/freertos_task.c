/**
 * @file freertos_task.c
 * @brief FreeRTOS 任务实现
 * @details 唯一的按键常驻任务 input_task：周期扫描按键，
 *          并在同一任务上下文中按按键 ID + 事件分发单击动作
 */

#include "bsp_includes.h"

/* ============================== 任务配置 ============================== */
/* 输入任务（按键扫描 + 事件分发）：基线栈深度，后续按实测高水位调整 */
#define INPUT_TASK_STACK        128
#define INPUT_TASK_PRIORITY     3

/* 扫描周期：与 multi_button 的 TICKS_INTERVAL(5ms) 保持一致 */
#define INPUT_SCAN_PERIOD_MS    TICKS_INTERVAL

/* ============================== 任务句柄 ============================== */
static TaskHandle_t input_task_handle = NULL;

/* ============================== 任务函数 ============================== */
static void input_task(void *pvParameters);

/**
 * @brief 启动 FreeRTOS 调度器
 * @note 在 main() 中初始化完成后调用；正常情况下不再返回
 */
void freertos_start(void)
{
    /* 调度器启动前直接创建唯一的按键常驻任务，省去 start_task 中转层 */
    if (xTaskCreate((TaskFunction_t)input_task,
                    "input_task",
                    (configSTACK_DEPTH_TYPE)INPUT_TASK_STACK,
                    NULL,
                    (UBaseType_t)INPUT_TASK_PRIORITY,
                    &input_task_handle) != pdPASS)
    {
        /* 创建失败：走明确的错误处理路径，不继续启动调度器 */
        Error_Handler();
    }

    vTaskStartScheduler();

    /* 仅当 heap 不足导致调度器启动失败时才会执行到这里 */
    Error_Handler();
}

/**
 * @brief 按键输入任务
 * @note  每 INPUT_SCAN_PERIOD_MS 调用一次 button_ticks()（消抖与单击识别），
 *        随后在同一任务上下文处理本轮产生的按键事件，全程非阻塞
 */
static void input_task(void *pvParameters)
{
    /* configTICK_RATE_HZ = 1000 时该值为 5；断言防止 tick 频率过低导致周期为 0 */
    const TickType_t scan_period = pdMS_TO_TICKS(INPUT_SCAN_PERIOD_MS);
    TickType_t last_wake_tick = xTaskGetTickCount();

    configASSERT(scan_period > 0);

    for (;;)
    {
        /* 1. 驱动 MultiButton 状态机 */
        button_ticks();

        /* 2. 分发本轮扫描产生的按键事件（KEY1 点亮 / KEY2 熄灭） */
        Key_ProcessEvents();

        /* 3. 固定节拍休眠，不忙等待、不阻塞 */
        xTaskDelayUntil(&last_wake_tick, scan_period);
    }
}
