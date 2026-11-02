/**
 * @file    delay.h
 * @brief   延时功能头文件
 * @details 提供基于 SysTick 和 DWT 的微秒级、毫秒级延时功能，支持阻塞和非阻塞模式
 *          包含系统滴答计数和 printf 重定向
 * @author  西伯利亚小菜狗
 * @date    2025-12-05
 * @version 2.0
 */

#ifndef __DELAY_H_
#define __DELAY_H_

/* ================================================================================== */
/* 配置选项                                                           */
/* ================================================================================== */

/**
 * @defgroup DELAY_CONFIG 延时模块配置
 * @{
 */

/**
 * @brief 选择使用的延时模式
 * @note  0: SysTick 模式 (适用于所有 Cortex-M 芯片)
 *        1: DWT 模式 (需芯片支持，精度更高，不占用 SysTick)
 */
#define USE_DWT_DELAY      1

/**
 * @brief 使能非阻塞延时功能
 * @note  1: 启用非阻塞延时 (需 SysTick 中断)
 *        0: 禁用非阻塞延时
 */
#define ENABLE_NON_BLOCKING_DELAY  0

/**
 * @brief 使能 printf 重定向
 * @note  1: 启用串口 printf 重定向
 *        0: 禁用重定向
 */
#define ENABLE_PRINTF_REDIRECT     0

/** @} */ /* 结束 DELAY_CONFIG 组 */

/* ================================================================================== */
/* 头文件包含                                                         */
/* ================================================================================== */

#include "bsp_includes.h"

/**
 * @addtogroup DELAY_DEFINES
 * @{
 */

/**
 * @brief 调试串口外设定义
 * @details 指定 printf 重定向使用的 USART 外设（本模板未开启串口，默认关闭）
 */
/* #define huart_debug USART2 */

/**
 * @brief 特殊延时值定义
 * @note  用于非阻塞延时的特殊标志
 */
#define DELAY_INFINITE     0xFFFFFFFF

/** @} */ /* 结束 DELAY_DEFINES 组 */

/* ================================================================================== */
/* 公共函数声明                                                       */
/* ================================================================================== */

/**
 * @defgroup DELAY_FUNCTIONS 延时函数接口
 * @{
 */

#if !USE_DWT_DELAY
/* --- SysTick 模式函数 --- */

/**
 * @brief   初始化 SysTick 延时环境
 * @param   sysclk_MHz 系统主频，单位 MHz (例如 72 表示 72MHz)
 * @retval  无
 * @note    自动配置 SysTick 中断，1ms 周期
 * @warning 如用户已编写 SysTick_Handler，需在中断中调用 g_sys_tick++
 */
void LL_delay_init(uint16_t sysclk_MHz);

/**
 * @brief   微秒级阻塞延时 (SysTick)
 * @param   us 延时的微秒数，范围 1~0xFFFFFFFF
 * @retval  无
 * @note    延时期间 CPU 无法执行其他任务
 */
void LL_delay_us(uint32_t us);

/**
 * @brief   毫秒级阻塞延时 (SysTick)
 * @param   ms 延时的毫秒数，范围 1~65535
 * @retval  无
 * @note    内部调用 LL_delay_us(1000)
 */
void LL_delay_ms(uint16_t ms);

#endif /* !USE_DWT_DELAY */

/* --- DWT 模式函数 --- */

#if USE_DWT_DELAY
/**
 * @brief   初始化 DWT 计数器
 * @note    务必在系统启动时调用一次
 */
void DWT_Init(void);

/**
 * @brief   DWT 微秒级阻塞延时
 * @param   us 要延时的微秒数
 * @note    基于 DWT 周期计数寄存器
 */
void DWT_Delay_us(uint32_t us);

/**
 * @brief   DWT 毫秒级阻塞延时
 * @param   ms 延时的毫秒数
 */
void DWT_Delay_ms(uint32_t ms);

#endif /* USE_DWT_DELAY */

/* --- 统一延时接口 (根据配置选择) --- */

/**
 * @brief   统一微秒延时接口
 * @details 根据 USE_DWT_DELAY 配置选择底层实现
 * @param   us 延时的微秒数
 */
#if USE_DWT_DELAY
#define delay_us(us)    DWT_Delay_us(us)
#else
#define delay_us(us)    LL_delay_us(us)
#endif

/**
 * @brief   统一毫秒延时接口
 * @details 根据 USE_DWT_DELAY 配置选择底层实现
 * @param   ms 延时的毫秒数
 */
#if USE_DWT_DELAY
#define delay_ms(ms)    DWT_Delay_ms(ms)
#else
#define delay_ms(ms)    LL_delay_ms(ms)
#endif

/* --- 非阻塞延时功能 (仅 SysTick 模式) --- */

#if !USE_DWT_DELAY && ENABLE_NON_BLOCKING_DELAY
/**
 * @brief   启动非阻塞毫秒延时
 * @param   nms 目标延时毫秒数
 * @retval  无
 * @warning 不可与阻塞版延时混用
 */
void LL_delay_ms_start(uint16_t nms);

/**
 * @brief   查询非阻塞延时是否完成
 * @param   无
 * @retval  1 延时已完成
 * @retval  0 延时未完成
 * @note    必须在 SysTick 中断正常工作的情况下使用
 */
uint8_t LL_delay_ms_check(void);
#endif /* !USE_DWT_DELAY && ENABLE_NON_BLOCKING_DELAY */

/* --- 时间获取函数 --- */

#if !USE_DWT_DELAY
/**
 * @brief   获取系统运行时间 (毫秒)
 * @param   无
 * @retval  当前 g_sys_tick 值
 * @note    关中断读取，确保线程安全
 */
uint32_t LL_GetTick(void);
#endif /* !USE_DWT_DELAY */

/* --- 运行时统计功能 (DWT 模式) --- */

#if USE_DWT_DELAY
/**
 * @brief   配置运行时间统计 (自适应频率)
 * @note    目标是将统计时钟频率控制在 20kHz 左右
 */
void ConfigureTimeForRunTimeStats(void);

/**
 * @brief   获取高精度时间值 (自适应位移)
 * @retval  运行时间计数值
 */
uint32_t GetRunTimeCounterValue(void);
#endif /* USE_DWT_DELAY */

/** @} */ /* 结束 DELAY_FUNCTIONS 组 */

/* ================================================================================== */
/* 公共变量声明                                                       */
/* ================================================================================== */

/**
 * @defgroup DELAY_VARIABLES 延时模块变量
 * @{
 */

#if !USE_DWT_DELAY
/**
 * @brief 系统滴答计数器
 * @details 毫秒级计数器，由 SysTick 中断每 1ms 递增一次
 */
extern volatile uint32_t g_sys_tick;
#endif /* !USE_DWT_DELAY */

#if USE_DWT_DELAY
/**
 * @brief CPU 统计移位位数
 * @details 用于自适应频率调节
 */
extern volatile uint8_t g_cpu_stats_shift;
#endif /* USE_DWT_DELAY */

/** @} */ /* 结束 DELAY_VARIABLES 组 */

/* ================================================================================== */
/* 中断服务函数声明                                                   */
/* ================================================================================== */

/**
 * @defgroup DELAY_ISR 中断服务函数
 * @{
 */

#if !USE_DWT_DELAY
/**
 * @brief SysTick 中断服务函数 (用户可选)
 * @note  如 MCU 启动文件已弱映射，请取消注释下方代码并调用 g_sys_tick++
 */
/* void SysTick_Handler(void) */
#endif /* !USE_DWT_DELAY */

/** @} */ /* 结束 DELAY_ISR 组 */

/* ================================================================================== */
/* 输出重定向功能                                                     */
/* ================================================================================== */

#if ENABLE_PRINTF_REDIRECT
/**
 * @defgroup REDIRECT_FUNCTIONS 输出重定向函数
 * @{
 */

/**
 * @brief   重定向 fputc 函数到 USART
 * @param   ch 要输出的字符
 * @param   f  文件指针 (标准库使用，忽略)
 * @retval  输出的字符
 * @note    需确保 huart_debug 已正确初始化
 */
int fputc(int ch, FILE *f);

/** @} */ /* 结束 REDIRECT_FUNCTIONS 组 */
#endif /* ENABLE_PRINTF_REDIRECT */

#endif /* __DELAY_H_ */
