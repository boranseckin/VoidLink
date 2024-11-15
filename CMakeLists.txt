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
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(sx126x_pico C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(sx126x_pico_tx sx126x_pico_tx.c)
add_executable(sx126x_pico_rx sx126x_pico_rx.c)

pico_set_program_name(sx126x_pico_tx "sx126x_pico_tx")
pico_set_program_name(sx126x_pico_rx "sx126x_pico_rx")
pico_set_program_version(sx126x_pico_tx "0.1")
pico_set_program_version(sx126x_pico_rx "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(sx126x_pico_tx 0)
pico_enable_stdio_usb(sx126x_pico_tx 1)

pico_enable_stdio_uart(sx126x_pico_rx 0)
pico_enable_stdio_usb(sx126x_pico_rx 1)

# Add the standard include files to the build
target_include_directories(sx126x_pico_tx PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_include_directories(sx126x_pico_rx PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

set(SX126X_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/SWSD003/sx126x/sx126x_driver)

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

    ${CMAKE_CURRENT_LIST_DIR}/src/pico_config.h
    ${CMAKE_CURRENT_LIST_DIR}/src/pico_gpio.h
    ${CMAKE_CURRENT_LIST_DIR}/src/pico_gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/pico_spi.h
    ${CMAKE_CURRENT_LIST_DIR}/src/pico_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/src/sx126x_hal_context.h
    ${CMAKE_CURRENT_LIST_DIR}/src/sx126x_hal.c
)

target_include_directories(sx126x INTERFACE
    ${SX126X_PATH}/src

    ${CMAKE_CURRENT_LIST_DIR}/src/
)

target_link_libraries(sx126x_pico_tx pico_stdlib hardware_spi sx126x)
target_link_libraries(sx126x_pico_rx pico_stdlib hardware_spi sx126x)

pico_add_extra_outputs(sx126x_pico_tx)
pico_add_extra_outputs(sx126x_pico_rx)

