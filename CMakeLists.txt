cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME FreeRTOS_Demo)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # BSP
    ${CMAKE_SOURCE_DIR}/BSP/Src/delay.c
    ${CMAKE_SOURCE_DIR}/BSP/Src/key_port.c
    ${CMAKE_SOURCE_DIR}/BSP/Src/multi_button.c
    ${CMAKE_SOURCE_DIR}/BSP/Src/freertos_task.c

    # FreeRTOS kernel (V11.3.3)
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/source/croutine.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/source/event_groups.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/source/list.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/source/queue.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/source/stream_buffer.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/source/tasks.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/source/timers.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/portable/GCC/ARM_CM3/port.c
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/portable/MemMang/heap_4.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/BSP/Inc
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/include
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/portable/GCC/ARM_CM3
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
