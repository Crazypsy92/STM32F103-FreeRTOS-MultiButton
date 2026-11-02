/**
 * @file key_port.c
 * @brief 按键驱动实现文件
 * @details 实现按键电平读取、单击事件记录，以及按按键 ID + 事件分发业务动作。
 *          事件记录在 button_ticks() 调用者的任务上下文中完成，非阻塞、无队列。
 * @author 西伯利亚小菜狗
 * @date 2025-12-05
 * @version 2.0
 * @copyright MIT
 */

#include "bsp_includes.h"

/* 按键对象 */
static Button KEY1;
static Button KEY2;

/* 本轮待处理的单击事件标志：bit1 对应 KEY_ID_1，bit2 对应 KEY_ID_2 */
#define KEY_PENDING_BIT(id)     ((uint8_t)(1U << (id)))
static uint8_t s_pending_clicks = 0;

/* 内部函数声明 */
static void key_record_event(uint8_t id, ButtonEvent event);
static void key_dispatch_event(uint8_t id, ButtonEvent event);

/**
 * @brief 读取引脚电平
 * @note 按键一端接 GND，GPIO 配置为上拉，按下为低电平
 */
uint8_t Read_Pin_Level(uint8_t button_id)
{
    switch (button_id)
    {
        case KEY_ID_1:
            return LL_GPIO_IsInputPinSet(key1_GPIO_Port, key1_Pin);
        case KEY_ID_2:
            return LL_GPIO_IsInputPinSet(key2_GPIO_Port, key2_Pin);
        default:
            return 1;   /* 默认返回释放电平 */
    }
}

/**
 * @brief 记录按键事件（非阻塞）
 * @note  只记录已注册按键的单击事件；未知 ID 或非单击事件直接忽略
 */
static void key_record_event(uint8_t id, ButtonEvent event)
{
    if (event != BTN_SINGLE_CLICK)
    {
        return;
    }

    switch (id)
    {
        case KEY_ID_1:
        case KEY_ID_2:
            s_pending_clicks |= KEY_PENDING_BIT(id);
            break;
        default:
            break;      /* 未知按键 ID：安全忽略 */
    }
}

/**
 * @brief 统一的按键回调函数
 * @note  运行在 button_ticks() 调用者的任务上下文（不是硬件中断），
 *        只做极短的事件记录，不发送队列、不阻塞
 */
void Key_Common_Callback(Button *btn)
{
    if (btn == NULL)
    {
        return;
    }

    key_record_event(btn->button_id, button_get_event(btn));
}

/**
 * @brief 按键业务分发
 * @note  KEY1 单击点亮 LED（LED 低电平点亮，输出低电平）；
 *        KEY2 单击熄灭 LED（输出高电平）；未知 ID / 事件不做任何动作
 */
static void key_dispatch_event(uint8_t id, ButtonEvent event)
{
    if (event != BTN_SINGLE_CLICK)
    {
        return;
    }

    switch (id)
    {
        case KEY_ID_1:
            LL_GPIO_ResetOutputPin(led_GPIO_Port, led_Pin);
            break;
        case KEY_ID_2:
            LL_GPIO_SetOutputPin(led_GPIO_Port, led_Pin);
            break;
        default:
            break;      /* 未知按键 ID：安全忽略 */
    }
}

/**
 * @brief 处理本轮扫描产生的按键事件
 * @details 由 input_task 在 button_ticks() 返回后调用。先取走并清空待处理标志，
 *          再按固定顺序分发：先 KEY1（点亮），后 KEY2（熄灭）。
 *          因此同一扫描周期内两键都触发单击时，最终 LED 为熄灭，
 *          与 MultiButton 回调的实际触发顺序无关。
 * @note  非阻塞、无锁、无动态内存
 */
void Key_ProcessEvents(void)
{
    uint8_t pending = s_pending_clicks;

    s_pending_clicks = 0;

    if (pending & KEY_PENDING_BIT(KEY_ID_1))
    {
        key_dispatch_event(KEY_ID_1, BTN_SINGLE_CLICK);
    }
    if (pending & KEY_PENDING_BIT(KEY_ID_2))
    {
        key_dispatch_event(KEY_ID_2, BTN_SINGLE_CLICK);
    }
}

/**
 * @brief 按键初始化
 * @note  必须在 FreeRTOS 调度器启动前调用；GPIO 由 MX_GPIO_Init 完成，
 *        此处只初始化 multi_button 对象并绑定必要事件
 */
void KEY_Init(void)
{
    /* 1. 初始化 multi_button 对象
     *    参数: handle, 读引脚函数, 有效电平(0=低电平有效), ID */
    button_init(&KEY1, Read_Pin_Level, 0, KEY_ID_1);
    button_init(&KEY2, Read_Pin_Level, 0, KEY_ID_2);

    /* 2. 仅绑定单击事件；长按等无业务需求的事件不启用，避免无效事件流 */
    button_attach(&KEY1, BTN_SINGLE_CLICK, Key_Common_Callback);
    button_attach(&KEY2, BTN_SINGLE_CLICK, Key_Common_Callback);

    /* 3. 启动按键 */
    button_start(&KEY1);
    button_start(&KEY2);
}
