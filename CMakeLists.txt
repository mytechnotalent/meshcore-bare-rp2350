# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# MIT License
#
# Copyright (c) 2026 Kevin Thomas
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Author:  Kevin Thomas
# Email:   kevin@mytechnotalent.com
# GitHub:  https://github.com/mytechnotalent/meshcore-bare-rp2350
# File:    CMakeLists.txt
# Desc:    Configures the RP2350 Pico 2 W MeshCore companion firmware build.
# Created: 2026

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(meshcore-bare-rp2350 C CXX ASM)

pico_sdk_init()

add_executable(meshcore-bare-rp2350
    crypto/ed25519/add_scalar.c
    crypto/ed25519/fe.c
    crypto/ed25519/ge.c
    crypto/ed25519/key_exchange.c
    crypto/ed25519/keypair.c
    crypto/ed25519/sc.c
    crypto/ed25519/seed.c
    crypto/ed25519/sha512.c
    crypto/ed25519/sign.c
    crypto/ed25519/verify.c
    src/identity.c
    src/mesh_crypto.c
    src/mesh_packet.c
    src/node_state.c
    src/advert.c
    src/datagram.c
    src/mesh_transport.c
    src/companion.c
    src/sx1262.c
    src/storage.c
    src/ble_companion.c
    src/runtime.c
    src/main.c
)

pico_set_program_name(meshcore-bare-rp2350 "meshcore-bare-rp2350")
pico_set_program_version(meshcore-bare-rp2350 "0.1")

pico_btstack_make_gatt_header(meshcore-bare-rp2350 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/meshcore.gatt)

target_include_directories(meshcore-bare-rp2350 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/crypto/ed25519
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(meshcore-bare-rp2350 PRIVATE
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_flash
    hardware_sync
    hardware_watchdog
    hardware_adc
    pico_rand
    pico_unique_id
    pico_cyw43_arch_none
    pico_btstack_ble
    pico_btstack_cyw43
    pico_mbedtls
)

pico_enable_stdio_uart(meshcore-bare-rp2350 0)
pico_enable_stdio_usb(meshcore-bare-rp2350 1)

pico_add_extra_outputs(meshcore-bare-rp2350)

add_custom_command(
    TARGET meshcore-bare-rp2350 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/meshcore-bare-rp2350.elf
            ${CMAKE_CURRENT_BINARY_DIR}/bare-meshcore-rp2350.elf
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/meshcore-bare-rp2350.uf2
            ${CMAKE_CURRENT_BINARY_DIR}/bare-meshcore-rp2350.uf2
    BYPRODUCTS
            ${CMAKE_CURRENT_BINARY_DIR}/bare-meshcore-rp2350.elf
            ${CMAKE_CURRENT_BINARY_DIR}/bare-meshcore-rp2350.uf2
    COMMENT "Generating backward-compatible bare-meshcore-rp2350 binaries"
    VERBATIM
)
