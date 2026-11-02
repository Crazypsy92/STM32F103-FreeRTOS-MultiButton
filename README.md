# FreeRTOS_Demo

A minimal FreeRTOS demo for the **STM32F103C8T6** (Cortex-M3) built with **CMake + Ninja** and the **ARM GCC** toolchain.

## Overview

This project demonstrates a lightweight, event-driven button input design on top of FreeRTOS. Two push-buttons (KEY1 / KEY2) control one on-board LED through a single persistent RTOS task. No queues, no dynamic allocations after startup, and no blocking calls in the button scan path.

- **MCU**: STM32F103C8T6, Cortex-M3 @ 72 MHz, 64 KB Flash / 20 KB RAM
- **RTOS**: FreeRTOS Kernel V11.3.3 (heap_4, 8 KB heap)
- **HAL/LL**: STM32F1 HAL + LL (GPIO uses LL for speed)
- **Time base**: TIM4 provides the HAL tick, leaving SysTick to FreeRTOS

## Hardware

| Signal | Pin   | Function                         |
|--------|-------|----------------------------------|
| LED    | PC13  | Active-low output (default OFF)  |
| KEY1   | PB12  | Input, pull-up, active-low       |
| KEY2   | PB14  | Input, pull-up, active-low       |

- **KEY1 single click** → LED ON
- **KEY2 single click** → LED OFF
- Long press and double click are intentionally not bound.

## Software Design

```
main()
 ├─ HAL_Init(), SystemClock_Config(), MX_GPIO_Init()
 ├─ DWT_Init()            // cycle counter for microsecond delays
 ├─ KEY_Init()            // init MultiButton objects, bind SINGLE_CLICK
 └─ freertos_start()      // create input_task, start scheduler (no return)

input_task (every 5 ms, xTaskDelayUntil)
 ├─ button_ticks()        // MultiButton debounce + click state machine
 └─ Key_ProcessEvents()   // dispatch recorded clicks by ID
      ├─ KEY1 + SINGLE_CLICK → LL_GPIO_ResetOutputPin(led)   // ON
      └─ KEY2 + SINGLE_CLICK → LL_GPIO_SetOutputPin(led)     // OFF
```

The button callback (`Key_Common_Callback`) runs in the `input_task` context (not an ISR) and only records a click into a bit-flag. `Key_ProcessEvents()` then dispatches the actions in a fixed order, so simultaneous clicks resolve deterministically.

## Directory Layout

```
.
├── BSP/                # Board support package (user code)
│   ├── Inc/            #   delay, key_port, multi_button, freertos_task, FreeRTOSConfig
│   └── Src/
├── Core/               # STM32CubeMX generated application code (main, gpio, it, ...)
├── Drivers/            # CMSIS + STM32F1xx HAL/LL driver
├── Middlewares/FreeRTOS/  # FreeRTOS kernel V11.3.3
├── cmake/              # Toolchain and CubeMX CMake fragments
├── CMakeLists.txt      # Top-level CMake
├── CMakePresets.json   # Debug / Release presets
├── STM32F103xx_FLASH.ld
├── startup_stm32f103xb.s
└── FreeRTOS_Demo.ioc   # STM32CubeMX project (for regeneration)
```

## Build

Prerequisites: CMake ≥ 3.22, Ninja, and `arm-none-eabi-gcc` on `PATH` (the STM32CubeIDE bundle is recommended).

```bash
# Configure (Debug)
cmake --preset Debug

# Build
cmake --build build/Debug
```

Output: `build/Debug/FreeRTOS_Demo.elf` (plus `.bin` / `.hex` if a post-build step is added).

Expected footprint (Debug):

```
RAM:   ~10 KB / 20 KB
FLASH: ~12 KB / 64 KB
```

## Flash & Debug

Use ST-Link with `STM32_Programmer_CLI`, OpenOCD, or the STM32CubeIDE debug launch configuration in `.vscode/launch.json`.

## Verification

| Check | Result |
| --- | --- |
| Debug build (CMake + Ninja + arm-none-eabi-gcc) | Pass |
| Host-simulation of the button state machine (mock GPIO, 5 ms tick loop) | Pass — KEY1 turns LED on, KEY2 turns LED off, simultaneous clicks resolve to OFF, long press ignored |
| Hardware (STM32F103C8T6, PC13 LED / PB12 KEY1 / PB14 KEY2) | Pass — scheduler runs, 5 ms scan period stable, LED responds to both buttons as expected |

Measured footprint (Debug build):

```
RAM:   10256 B / 20 KB   (50.08%)
FLASH: 12148 B / 64 KB   (18.54%)
```

## Configuration Notes

- `configTICK_RATE_HZ = 1000`, scan period = 5 ms, matching `TICKS_INTERVAL` in `multi_button.h`.
- `configMAX_PRIORITIES = 5`; `input_task` runs at priority 3.
- Software timers, trace, and run-time stats are disabled to keep the footprint small.
- `FreeRTOSConfig.h` maps `vPortSVCHandler` → `SVC_Handler` and `xPortPendSVHandler` → `PendSV_Handler`; `SysTick_Handler` calls `xPortSysTickHandler()`.

## License

User code under `BSP/` is released under the **MIT License**. See [LICENSE](LICENSE). STM32 HAL/LL and FreeRTOS sources retain their own licenses.
