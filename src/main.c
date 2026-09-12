// MIT License
//
// Copyright (c) 2026 Kevin Thomas
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Author:  Kevin Thomas
// Email:   kevin@mytechnotalent.com
// GitHub:  https://github.com/mytechnotalent/meshcore-bare-rp2350
// File:    main.c
// Desc:    Initializes the bare MeshCore application entry point for RP2350.
// Created: 2026

#include "ble_companion.h"
#include "config.h"
#include "node_state.h"
#include "pico/cyw43_arch.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "runtime.h"
#include "storage.h"
#include "sx1262.h"

#include <stdio.h>

/**
 * @brief Application main entry point for RP2350 Pico 2 W.
 *
 * Initializes stdio, CYW43 architecture, node state, flash persistence,
 * BTstack BLE companion service, SX1262 LoRa transceiver, starts BLE
 * advertising, and runs the main radio and companion runtime loop.
 *
 * @param void No parameters.
 * @return int Exit code (0 on normal termination).
 */
int main(void) {
    /**
     * @brief Declaration of start_us.
     */
    uint32_t start_us;
    stdio_init_all();
    start_us = time_us_32();
    while (!stdio_usb_connected()) {
        sleep_ms(50U);
        if ((time_us_32() - start_us) > 15000000U) {
            break;
        }
    }
    if (cyw43_arch_init() != 0) {
        printf("[main] CYW43 architecture initialization failed\n");
        return -1;
    }
    node_state_init();
    storage_init();
    ble_companion_init();
    if (!sx1262_init(MESHCORE_DEFAULT_FREQUENCY,
                     MESHCORE_DEFAULT_BANDWIDTH,
                     MESHCORE_DEFAULT_SPREADING_FACTOR,
                     MESHCORE_DEFAULT_CODING_RATE)) {
        printf("[main] SX1262 LoRa radio initialization failed\n");
    }
    ble_companion_start();
    runtime_run();
    return 0;
}
