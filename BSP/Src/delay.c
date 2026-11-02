/**
 * @file    delay.c
 * @brief   延时功能实现文件
 * @details 根据配置提供 SysTick 或 DWT 延时实现，支持阻塞和非阻塞模式
 * @author  西伯利亚小菜狗
 * @date    2025-12-05
 * @version 2.0
 */

#include "bsp_includes.h"

/* ================================================================================== */
/* 宏定义与配置检查                                                   */
/* ================================================================================== */

#if USE_DWT_DELAY && ENABLE_NON_BLOCKING_DELAY
#error "非阻塞延时仅支持 SysTick 模式，请将 ENABLE_NON_BLOCKING_DELAY 设为 0"
#endif

#if !USE_DWT_DELAY && !ENABLE_NON_BLOCKING_DELAY
#pragma message("非阻塞延时已禁用,LL_delay_ms_start/check 函数不可用")
#endif

/* ================================================================================== */
/* SysTick 延时实现 (USE_DWT_DELAY=0 时启用)                             */
/* ================================================================================== */

#if !USE_DWT_DELAY

/* --- 私有变量定义 --- */

/** @brief 微秒级延时因子 */
static uint32_t g_fac_us = 0;

/** @brief 毫秒级延时因子 */
static uint32_t g_fac_ms = 0;

/** @brief 系统滴答计数器，毫秒级 */
volatile uint32_t g_sys_tick = 0;

/* --- 非阻塞延时相关变量 --- */

#if ENABLE_NON_BLOCKING_DELAY
/** @brief 非阻塞延时开始时的滴答值 */
static uint32_t g_delay_start_tick = 0;

/** @brief 非阻塞延时目标滴答值 */
static uint32_t g_delay_target_tick = 0;

/** @brief 非阻塞延时进行中标志 */
static uint8_t g_delay_in_progress = 0;
#endif /* ENABLE_NON_BLOCKING_DELAY */

/* ================================================================================== */
/* SysTick 公共函数实现                                                */
/* ================================================================================== */

/**
 * @brief   初始化 SysTick 延时环境
 * @param   sysclk_MHz 系统主频，单位 MHz (例如 72 表示 72MHz)
 * @retval  无
 */
void LL_delay_init(uint16_t sysclk_MHz)
{
    SysTick->CTRL = 0;                               /* 先关掉 SysTick */
    SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk;    /* 选择 HCLK/8 */
    g_fac_us = sysclk_MHz / 8;                       /* 计算 1 us 需要的 tick 数 */
    
    /* 配置 SysTick 每 1ms 产生一次中断 */
    SysTick->LOAD = (uint32_t)g_fac_us * 1000;       /* 1ms 对应的 tick 数 */
    SysTick->VAL  = 0UL;                             /* 清空计数器 */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;       /* 使能中断 */
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;        /* 启动计数器 */
    
    g_sys_tick = 0;                                  /* 初始化滴答计数器 */
}

/**
 * @brief   微秒级阻塞延时 (SysTick)
 * @param   nus 延时的微秒数，范围 1~0xFFFFFFFF
 * @retval  无
 */
void LL_delay_us(uint32_t nus)
{
    uint32_t ticks = nus * g_fac_us;
    uint32_t temp;
    uint32_t old_ctrl = SysTick->CTRL;               /* 保存当前控制寄存器值 */

    /* 关闭 SysTick 中断，避免干扰阻塞延时 */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
    
    SysTick->LOAD = ticks;
    SysTick->VAL  = 0UL;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    /* 等待计数完成 */
    do {
        temp = SysTick->CTRL;
    } while ((temp & SysTick_CTRL_ENABLE_Msk) && !(temp & SysTick_CTRL_COUNTFLAG_Msk));

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL   = 0UL;
    
    /* 恢复中断配置 */
    SysTick->CTRL = old_ctrl;
}

/**
 * @brief   毫秒级阻塞延时 (SysTick)
 * @param   nms 延时的毫秒数，范围 1~65535
 * @retval  无
 */
void LL_delay_ms(uint16_t nms)
{
    uint32_t repeat = nms / 1000U;
    uint32_t remain = nms % 1000U;

    while (repeat--) {
        LL_delay_us(1000000UL);
    }
    if (remain) {
        LL_delay_us(remain * 1000UL);
    }
}

/* ================================================================================== */
/* 非阻塞延时函数实现 (ENABLE_NON_BLOCKING_DELAY=1 时启用)               */
/* ================================================================================== */

#if ENABLE_NON_BLOCKING_DELAY

/**
 * @brief   启动非阻塞毫秒延时
 * @param   nms 目标延时毫秒数
 * @retval  无
 */
void LL_delay_ms_start(uint16_t nms)
{
    g_delay_start_tick = g_sys_tick;                /* 记录当前系统滴答值 */
    g_delay_target_tick = nms;
    g_delay_in_progress = 1;
}

/**
 * @brief   查询非阻塞延时是否完成
 * @param   无
 * @retval  1 延时已完成
 * @retval  0 延时未完成
 */
uint8_t LL_delay_ms_check(void)
{
    if (!g_delay_in_progress) return 1;             /* 未启动延时，视为已完成 */
    
    uint32_t current_tick = g_sys_tick;
    
    /* 处理滴答计数器溢出 */
    if (current_tick >= g_delay_start_tick) {
        if ((current_tick - g_delay_start_tick) >= g_delay_target_tick) {
            g_delay_in_progress = 0;
            return 1;
        }
    } else {
        /* 发生溢出回绕 */
        uint32_t elapsed = (0xFFFFFFFF - g_delay_start_tick) + current_tick;
        if (elapsed >= g_delay_target_tick) {
            g_delay_in_progress = 0;
            return 1;
        }
    }
    return 0;
}

#endif /* ENABLE_NON_BLOCKING_DELAY */

/* ================================================================================== */
/* 时间获取函数实现                                                   */
/* ================================================================================== */

/**
 * @brief   获取系统运行时间 (毫秒)
 * @param   无
 * @retval  当前 g_sys_tick 值
 */
uint32_t LL_GetTick(void)
{
    uint32_t tick;
    
    /* 关闭中断防止读取时发生溢出 */
    __disable_irq();
    tick = g_sys_tick;
    __enable_irq();
    
    return tick;
}

#endif /* !USE_DWT_DELAY */

/* ================================================================================== */
/* DWT 延时实现 (USE_DWT_DELAY=1 时启用)                               */
/* ================================================================================== */

#if USE_DWT_DELAY

/* --- DWT 寄存器地址定义 --- */

/**
 * @defgroup DWT_REGISTERS DWT 寄存器定义
 * @{
 */

#define DWT_CYCCNT  *(volatile uint32_t *)0xE0001004 /**< 周期计数寄存器 */
#define DWT_CTRL    *(volatile uint32_t *)0xE0001000 /**< DWT 控制寄存器 */
#define DEMCR       *(volatile uint32_t *)0xE000EDFC /**< 调试异常评测控制寄存器 */

/** @} */ /* 结束 DWT_REGISTERS 组 */

/* --- 私有变量定义 --- */

/**
 * @brief DWT 溢出计数 (32位->64位扩展)
 */
static volatile uint32_t g_dwt_overflow_count = 0;

/**
 * @brief DWT 上次计数值 (用于溢出检测)
 */
static volatile uint32_t g_dwt_last_value = 0;

/**
 * @brief CPU 统计移位位数 (用于自适应频率调节)
 */
volatile uint8_t g_cpu_stats_shift = 0;

/* --- 外部声明 --- */
extern uint32_t SystemCoreClock; /**< 系统核心时钟频率 */

/* ================================================================================== */
/* DWT 公共函数实现                                                   */
/* ================================================================================== */

/**
 * @brief   初始化 DWT 计数器
 * @note    务必在系统启动时 (如 main 函数开头) 调用一次
 */
void DWT_Init(void) 
{
    DEMCR |= 0x01000000;  /* 使能 DWT */
    DWT_CYCCNT = 0;       /* 清零计数器 */
    DWT_CTRL |= 1;        /* 启动计数 */
    
    /* 初始化溢出记录 */
    g_dwt_overflow_count = 0;
    g_dwt_last_value = 0;
}

/**
 * @brief   DWT 微秒级阻塞延时
 * @param   us 要延时的微秒数
 */
void DWT_Delay_us(uint32_t us) 
{
    uint32_t start_tick = DWT_CYCCNT;
    
    /* 计算需要的时钟周期数：us * (SystemCoreClock / 1000000) */
    uint32_t delay_ticks = us * (SystemCoreClock / 1000000);

    /* 循环等待，利用无符号数减法特性，天然处理溢出回绕问题 */
    while ((DWT_CYCCNT - start_tick) < delay_ticks);
}

/**
 * @brief   DWT 毫秒级阻塞延时
 * @param   ms 延时的毫秒数
 */
void DWT_Delay_ms(uint32_t ms) 
{
    uint32_t start_tick = DWT_CYCCNT;
    
    /* SystemCoreClock 是 Hz，除以 1000 就是每毫秒的周期数 */
    uint32_t delay_ticks = ms * (SystemCoreClock / 1000); 
    
    while ((DWT_CYCCNT - start_tick) < delay_ticks);
}

/* ================================================================================== */
/* 运行时统计功能实现                                                 */
/* ================================================================================== */

/**
 * @brief   内部函数：获取 64 位 DWT 计数 (包含溢出处理)
 * @retval  64 位的 DWT 周期计数
 */
static uint64_t DWT_Get64bitCount(void) 
{
    uint32_t current = DWT_CYCCNT;
    
    /* 检测溢出 (当前值 < 上次值，说明溢出一次) */
    if (current < g_dwt_last_value) {
        g_dwt_overflow_count++;
    }
    g_dwt_last_value = current;
    
    /* 组合为 64 位：溢出次数<<32 + 当前值 */
    return ((uint64_t)g_dwt_overflow_count << 32) | current;
}

/**
 * @brief   配置运行时间统计 (自适应频率版)
 * @note    目标是将统计时钟频率控制在 20kHz 左右
 */
void ConfigureTimeForRunTimeStats(void)
{
    uint32_t target_freq = 20000; /* 目标频率 20kHz (50us 精度) */
    uint32_t ratio;

    /* 1. 确保 DWT 硬件已开启 */
    DWT_Init(); 

    /* 2. 获取当前最新的 CPU 频率 */
    extern void SystemCoreClockUpdate(void);
    SystemCoreClockUpdate();

    /* 3. 计算分频比 (例如: 72MHz / 20kHz = 3600) */
    if (target_freq > SystemCoreClock) {
        target_freq = SystemCoreClock; /* 防止除 0 */
    }
    ratio = SystemCoreClock / target_freq;

    /* 4. 计算最接近的 2 的幂次方 (即右移位数) */
    g_cpu_stats_shift = 0;
    while (ratio > 1) {
        ratio >>= 1;
        g_cpu_stats_shift++;
    }
    
    /* 调试信息输出 (可选) */
    /* printf("CPU Freq: %u Hz, Stats Shift: %d\r\n", SystemCoreClock, g_cpu_stats_shift); */
}

/**
 * @brief   获取高精度时间值 (自适应位移)
 * @retval  运行时间计数值
 */
uint32_t GetRunTimeCounterValue(void) 
{
    /* 先获取 64 位总计数，再右移分频 */
    uint64_t total_count = DWT_Get64bitCount();
    return (uint32_t)(total_count >> g_cpu_stats_shift);
}



#endif /* USE_DWT_DELAY */

/* ================================================================================== */
/* 输出重定向功能实现                                                 */
/* ================================================================================== */

#if ENABLE_PRINTF_REDIRECT

/**
 * @brief   重定向 fputc 函数到 USART
 * @param   ch 要输出的字符
 * @param   f  文件指针 (标准库使用，忽略)
 * @retval  输出的字符
 */
int fputc(int ch, FILE *f)
{
    (void)f; /* 避免未使用参数警告 */
    
    /* 等待直到 TXE 标志置位 */
    while (!LL_USART_IsActiveFlag_TXE(huart_debug)) { }
    
    LL_USART_TransmitData8(huart_debug, (uint8_t)ch);
    return ch;
}

#endif /* ENABLE_PRINTF_REDIRECT */

/* ================================================================================== */
/* 文件结束                                                           */
/* ================================================================================== */

/********************************************************
  End Of File
********************************************************/
