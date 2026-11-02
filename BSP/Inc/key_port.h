/**
 * @file key_port.h
 * @brief 按键驱动接口头文件
 * @details 定义按键硬件接口及相关函数声明，基于 multi_button 库实现
 * @author 西伯利亚小菜狗
 * @date 2025-12-05
 * @version 2.0
 * @copyright MIT
 */

#ifndef __KEY_PORT_H_
#define __KEY_PORT_H_

#include "bsp_includes.h"
#include "multi_button.h"

/**
 * @defgroup KEY_CONFIG 按键硬件配置
 * @brief 按键 ID 及硬件引脚映射定义
 * @{
 */
#define KEY_ID_1    1   /**< key1: PB12 */
#define KEY_ID_2    2   /**< key2: PB14 */
/** @} */

/**
 * @brief 按键初始化函数
 * @details 初始化 multi_button 对象，并绑定单击事件回调
 * @note 按键 GPIO 由 MX_GPIO_Init 初始化；需在 FreeRTOS 调度器启动前调用
 */
void KEY_Init(void);

/**
 * @brief 读取按键引脚电平（供 multi_button 调用）
 * @param[in] button_id 按键 ID（对应 KEY_ID_x 定义）
 * @retval 0 按键按下（低电平）
 * @retval 1 按键释放（高电平）
 */
uint8_t Read_Pin_Level(uint8_t button_id);

/**
 * @brief 按键事件统一回调函数
 * @details 在 button_ticks() 中被调用（input_task 任务上下文，非硬件中断），
 *          只记录本轮按键事件，不阻塞、不发送队列
 * @param[in] btn 指向按键对象的指针
 */
void Key_Common_Callback(Button *btn);

/**
 * @brief 处理本轮扫描产生的按键事件
 * @details 由 input_task 在 button_ticks() 返回后调用，按 ID + 事件分发：
 *          KEY1 单击点亮 LED，KEY2 单击熄灭 LED。
 *          同一周期内两键均触发时以 KEY2（熄灭）为最终动作。
 * @note 非阻塞，可在任务上下文中直接调用
 */
void Key_ProcessEvents(void);

#endif /* __KEY_PORT_H_ */
