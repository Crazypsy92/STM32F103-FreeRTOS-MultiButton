# 按键模块轻量化重构报告（方案 A）

日期：2026-11-01
范围：`BSP/Src/freertos_task.c`、`BSP/Src/key_port.c`、`BSP/Inc/key_port.h`、`Core/Src/main.c`（仅注释）

## 1. 变更文件

| 文件 | 变更 |
|---|---|
| `BSP/Src/freertos_task.c` | 删除 `start_task`、`key_scan_task`、`key_task`；`freertos_start()` 直接创建唯一 `input_task`；检查 `xTaskCreate` 返回值，失败走 `Error_Handler()`；扫描使用 `xTaskDelayUntil()` 固定 5 ms 节拍，并在 `button_ticks()` 后调用 `Key_ProcessEvents()` |
| `BSP/Src/key_port.c` | 删除 `KeyQueue`/`KeyEvent_t` 事件与队列收发；回调只做非阻塞事件记录；新增 `Key_ProcessEvents()` 按 ID + Event 分发；`KEY_Init()` 只初始化对象并绑定 `BTN_SINGLE_CLICK`；长按不绑定 |
| `BSP/Inc/key_port.h` | 删除 `KeyEvent_t`、`extern KeyQueue`；新增 `void Key_ProcessEvents(void)` 声明；更新注释 |
| `Core/Src/main.c` | 仅更新 `KEY_Init()` 的注释，初始化顺序与 HAL/LL/时钟配置未改动 |

源码注释使用中文；文件名、符号名保持英文。

## 2. 原结构与新结构

改造前：

```
freertos_start()
 └─ xTaskCreate(start_task, 128)
      ├─ xTaskCreate(key_scan_task, 128)  每 5 ms button_ticks()
      └─ xTaskCreate(key_task, 256)       阻塞 xQueueReceive(KeyQueue)
                                             └─ 仅按 evt.event 判断 SINGLE_CLICK → 翻转 LED
KEY_Init(): xQueueCreate(10, KeyEvent_t) + 绑定 SINGLE_CLICK / LONG_PRESS_START
```

改造后：

```
freertos_start()
 └─ xTaskCreate(input_task, 128) 失败 → Error_Handler()
 └─ vTaskStartScheduler()       返回（heap 不足）→ Error_Handler()

input_task（每 5 ms，xTaskDelayUntil）
 ├─ button_ticks()（MultiButton 消抖/单击识别）
 └─ Key_ProcessEvents()
      ├─ KEY_ID_1 + BTN_SINGLE_CLICK → LL_GPIO_ResetOutputPin(led)  // 低电平点亮
      └─ KEY_ID_2 + BTN_SINGLE_CLICK → LL_GPIO_SetOutputPin(led)    // 高电平熄灭

KEY_Init(): 初始化 KEY1/KEY2，仅绑定 BTN_SINGLE_CLICK
```

事件处理方式：回调 `Key_Common_Callback()` 只把单击事件记录到 `s_pending_clicks` 位标志（bit1=KEY1，bit2=KEY2），`Key_ProcessEvents()` 在 `button_ticks()` 返回后按「先 KEY1、后 KEY2」的固定顺序处理。**不使用队列**，无动态内存，无锁。

冲突处理（A07）：同一扫描周期内两键均产生单击时，最终动作固定为 KEY2 的「熄灭」，与 MultiButton 回调实际触发顺序无关（MultiButton 的按键链表为后注册先遍历，实际回调顺序是 KEY2 → KEY1，测试已验证最终仍为熄灭）。

## 3. LED 电平核对

- `main.h`: `led_Pin = LL_GPIO_PIN_13`, `led_GPIO_Port = GPIOC`
- `gpio.c`: 初始化前先 `LL_GPIO_SetOutputPin()`（高电平），因此上电默认熄灭
- 低电平点亮 → ON 用 `LL_GPIO_ResetOutputPin()`，OFF 用 `LL_GPIO_SetOutputPin()`；已按实际配置实现，未混淆逻辑状态与电气电平

## 4. 节拍配置核对

- `configTICK_RATE_HZ = 1000`，`pdMS_TO_TICKS(5) = 5`，不为 0
- `multi_button.h`: `TICKS_INTERVAL = 5 ms`，扫描周期与之一致
- `FreeRTOSConfig.h`: `INCLUDE_vTaskDelayUntil = 1`；FreeRTOS V11.3.3 中映射为 `INCLUDE_xTaskDelayUntil`，`xTaskDelayUntil()` 可用
- 运行时用 `configASSERT(scan_period > 0)` 兜底

## 5. 测试结果

### 5.1 构建（A09）

```
cmake --build build/Debug
RAM:   10256 B / 20 KB (50.08%)
FLASH: 12148 B / 64 KB (18.54%)
```

无新增编译/链接错误。`arm-none-eabi-nm -u` 检查无 `KeyQueue`/`key_task`/`key_scan_task` 未解析引用；应用层仅剩 1 处 `xTaskCreate`。

### 5.2 宿主模拟单元测试（PC，mock GPIO，无 FreeRTOS）

使用**真实的** `key_port.c` 与 `multi_button.c`，仅替换 GPIO 读/写函数，按 5 ms 节拍循环调用 `button_ticks()` + `Key_ProcessEvents()`。测试程序位于 `/tmp/keyref_host_test/`（临时辅助验证，未纳入工程，因为工程当前无测试框架）。

| 用例 | 操作 | 期望 | 结果 |
|---|---|---|---|
| A02 | KEY1 单击 / 重复单击 | ON / 保持 ON | PASS（宿主模拟） |
| A03 | KEY2 单击 / 重复单击 | OFF / 保持 OFF | PASS（宿主模拟） |
| A04 | KEY1→KEY2→KEY1 | ON→OFF→ON | PASS（宿主模拟） |
| A05 | KEY1 / KEY2 长按 | 无动作 | PASS（宿主模拟） |
| A07 | 两键同周期单击 | 最终 OFF | PASS（宿主模拟，含重复验证） |
| A08 | 高频交替按键 10 轮 | 状态一致、无卡死 | PASS（宿主模拟） |
| A06 | 未知 ID / 未处理事件 | 安全忽略 | PASS（静态检查：`key_record_event`/`key_dispatch_event` 的 `default` 分支直接返回，仅记录 SINGLE_CLICK） |
| A01 | 上电启动、任务数量 | 仅 1 个按键常驻任务 | **NOT TESTED（硬件）**；静态检查：应用层只创建 `input_task`，另有内核 Idle 任务（`configUSE_TIMERS = 0`，无定时器任务） |

**HARDWARE NOT TESTED**：未连接 STM32F103C8T6 实板，上电、实体按键、实时调度、真实 RAM 占用均未实测，不得视为硬件 PASS。

### 5.3 内存与任务对比（静态推算，未实测）

| 项目 | 改造前 | 改造后 |
|---|---|---|
| 应用常驻任务 | 3（key_scan、key_task + Idle） | 2（input_task + Idle） |
| 按键相关常驻任务 | 2 | 1 |
| 按键任务栈预算（4 B/单位） | (128+256)×4 = 1536 B | 128×4 = 512 B |
| 启动中转任务 | start_task 128 单位 = 512 B（启动后删除） | 无 |
| 按键事件队列 | 1 个（深度 10，元素 2 B，另有队列结构与 heap 头，约 120 B） | 无 |
| 峰值 heap 估算（含 Idle 608 B） | ≈ 3.0 KB | ≈ 1.2 KB |
| 稳态 heap 估算（start_task 已回收） | ≈ 2.4 KB | ≈ 1.2 KB |

说明：
- 栈单位为 `StackType_t` = 4 B；TCB 按本工程配置（mutex/notification 开启、无 trace）估算约 88 B，heap_4 另有块头。
- 上述为按配置与结构体布局的推算，**不是运行时实测**，不作为优化承诺。
- ELF 静态 RAM 与改造前几乎相同（仅少 1 个 4 B 队列句柄、多 1 B 标志位），节省主要体现在运行时 heap。

运行时内存测量（A10）：**NOT TESTED**，原因如下：
- `uxTaskGetStackHighWaterMark` 未开启（`INCLUDE_uxTaskGetStackHighWaterMark` 未定义，FreeRTOS 默认 0）
- `configCHECK_FOR_STACK_OVERFLOW = 0`，无栈溢出检测
- `configUSE_TRACE_FACILITY = 0`、`configGENERATE_RUN_TIME_STATS = 0`，无运行统计
- 无实板，无法调用 `xPortGetFreeHeapSize()` 读取剩余 heap

## 6. 未测试项

| 项 | 状态 |
|---|---|
| A01 上电启动 / 实际任务列表 | HARDWARE NOT TESTED |
| A02–A05、A07、A08 实板物理按键 | HARDWARE NOT TESTED（仅有宿主模拟 PASS） |
| 实际扫描节拍抖动 | HARDWARE NOT TESTED |
| A10 栈高水位、剩余 heap、最大任务数 | NOT TESTED（配置不支持测量且无实板） |
| 栈溢出检测 | NOT TESTED（`configCHECK_FOR_STACK_OVERFLOW = 0`） |

## 7. 队列、阻塞与扩展说明

- 本轮**不使用队列**，`Key_Common_Callback()` 运行在 `button_ticks()` 调用者的任务上下文（非硬件 ISR），只做位标志记录，确定时间、非阻塞。
- `Key_ProcessEvents()` 只执行 LL GPIO 写，无阻塞风险，未调用 `vTaskDelay()`、阻塞队列、Flash 等。
- 未来串口发送、Flash 写入、网络请求、耗时算法等：**不要**放进 `button_ticks()` 回调，也**不要**为此新建「按键处理任务」。应通过非阻塞队列 / 任务通知，把 `(id, event)` 交给已有的业务任务处理；`input_task` 只负责识别与转发。
- 本轮栈保持 128 单位基线；建议后续在实板上开启 `INCLUDE_uxTaskGetStackHighWaterMark = 1`（及 `configCHECK_FOR_STACK_OVERFLOW = 2`）测量高水位后再决定是否调整。
