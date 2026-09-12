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
// File:    config.h
// Desc:    Hardware and protocol defaults for the RP2350 MeshCore companion.
// Created: 2026

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/**
 * @brief SX1262 SPI clock pin on RP2350.
 */
#define MESHCORE_PIN_SCK 10u

/**
 * @brief SX1262 SPI MOSI pin on RP2350.
 */
#define MESHCORE_PIN_MOSI 11u

/**
 * @brief SX1262 SPI MISO pin on RP2350.
 */
#define MESHCORE_PIN_MISO 12u

/**
 * @brief SX1262 chip-select pin on RP2350.
 */
#define MESHCORE_PIN_NSS 3u

/**
 * @brief SX1262 reset pin on RP2350.
 */
#define MESHCORE_PIN_RESET 15u

/**
 * @brief SX1262 busy pin on RP2350.
 */
#define MESHCORE_PIN_BUSY 2u

/**
 * @brief SX1262 DIO1 interrupt pin on RP2350.
 */
#define MESHCORE_PIN_DIO1 20u

/**
 * @brief RF switch power control pin on RP2350.
 */
#define MESHCORE_PIN_ANT_SW 22u

/**
 * @brief Battery voltage ADC input pin on RP2350.
 */
#define MESHCORE_PIN_BATTERY_ADC 26u

/**
 * @brief Companion BLE device name.
 */
#define MESHCORE_DEVICE_NAME "MeshCore-Bare"

/**
 * @brief Companion BLE pairing PIN.
 */
#define MESHCORE_BLE_PIN 123456U

/**
 * @brief Maximum MeshCore group channels.
 */
#define MESHCORE_MAX_CHANNELS 40U

/**
 * @brief Default radio frequency in hertz.
 */
#define MESHCORE_DEFAULT_FREQUENCY 910525000U

/**
 * @brief Default LoRa bandwidth in hertz.
 */
#define MESHCORE_DEFAULT_BANDWIDTH 62500U

/**
 * @brief Default LoRa spreading factor.
 */
#define MESHCORE_DEFAULT_SPREADING_FACTOR 7U

/**
 * @brief Default LoRa coding rate denominator.
 */
#define MESHCORE_DEFAULT_CODING_RATE 5U

#endif // CONFIG_H
