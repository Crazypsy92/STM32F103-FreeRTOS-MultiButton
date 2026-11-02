/**
 * @file bsp_includes.h
 * @brief BSP 总头文件
 * @details 包含系统基础头文件、标准库头文件、FreeRTOS 及自定义模块头文件，
 *          作为应用层的统一头文件入口（由 main.h 包含）
 * @author 西伯利亚小菜狗
 * @date 2025-12-05
 * @version 1.0
 */
#ifndef __BSP_INCLUDES_H__
#define __BSP_INCLUDES_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

#include "freertos_task.h"
#include "delay.h"
#include "key_port.h"
#include "multi_button.h"

#define DEBUG 1
#if DEBUG
#define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...) do {} while (0)
#endif

#ifndef App_Printf
#define App_Printf(...) printf(__VA_ARGS__)
#endif

#endif /* __BSP_INCLUDES_H__ */
