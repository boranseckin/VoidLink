# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(voidlink C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(voidlink src/voidlink.c)

pico_set_program_name(voidlink "voidlink")
pico_set_program_version(voidlink "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(voidlink 0)
pico_enable_stdio_usb(voidlink 1)

# Add the standard include files to the build
target_include_directories(voidlink PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

set(SX126X_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/sx126x_driver)
set(EPAPER_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/epaper_driver)

# ePaper driver
add_subdirectory(${EPAPER_PATH}/Config)
add_subdirectory(${EPAPER_PATH}/e-Paper)
add_subdirectory(${EPAPER_PATH}/Fonts)
add_subdirectory(${EPAPER_PATH}/GUI)

include_directories(${EPAPER_PATH}/Config)
include_directories(${EPAPER_PATH}/e-Paper)
include_directories(${EPAPER_PATH}/GUI)

# SX126x driver
add_library(sx126x INTERFACE)

target_sources(sx126x INTERFACE
    ${SX126X_PATH}/src/lr_fhss_mac.c
    ${SX126X_PATH}/src/lr_fhss_mac.h
    ${SX126X_PATH}/src/lr_fhss_v1_base_types.h
    ${SX126X_PATH}/src/sx126x_driver_version.c
    ${SX126X_PATH}/src/sx126x_driver_version.h
    ${SX126X_PATH}/src/sx126x_hal.h
    ${SX126X_PATH}/src/sx126x_lr_fhss.c
    ${SX126X_PATH}/src/sx126x_lr_fhss.h
    ${SX126X_PATH}/src/sx126x_regs.h
    ${SX126X_PATH}/src/sx126x.c
    ${SX126X_PATH}/src/sx126x.h

    # Pico HAL files
    ${CMAKE_CURRENT_LIST_DIR}/src/pico/pico_config.h
    ${CMAKE_CURRENT_LIST_DIR}/src/pico/pico_gpio.h
    ${CMAKE_CURRENT_LIST_DIR}/src/pico/pico_gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/pico/pico_spi.h
    ${CMAKE_CURRENT_LIST_DIR}/src/pico/pico_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/src/pico/sx126x_hal_context.h
    ${CMAKE_CURRENT_LIST_DIR}/src/pico/sx126x_hal.c
)

target_include_directories(sx126x INTERFACE
    ${SX126X_PATH}/src

    ${CMAKE_CURRENT_LIST_DIR}/src/
    ${CMAKE_CURRENT_LIST_DIR}/src/pico
)

# Final link
target_link_libraries(voidlink pico_stdlib pico_multicore hardware_spi sx126x ePaper GUI Fonts Config)

pico_add_extra_outputs(voidlink)
