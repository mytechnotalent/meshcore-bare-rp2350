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
// File:    sx1262.c
// Desc:    Implements the SX1262 SPI LoRa radio driver for RP2350.
// Created: 2026

#include "sx1262.h"

#include "config.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/regs/spi.h"
#include "hardware/spi.h"
#include "hardware/structs/spi.h"
#include "mesh_packet.h"
#include "pico/mutex.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Mutex protecting SX1262 hardware and state.
 */
static mutex_t _sx1262_mutex;

/**
 * @brief Flag indicating if the SX1262 mutex has been initialized.
 */
static bool _sx1262_mutex_initialized = false;

/**
 * @brief Flag indicating if the SX1262 is ready and operating.
 */
static bool _sx1262_ready = false;

/**
 * @brief SNR times four of last received packet.
 */
static int8_t _last_snr_x4 = 0;

/**
 * @brief RSSI in dBm of last received packet.
 */
static int8_t _last_rssi = -60;

/**
 * @brief Wait until the SX1262 releases its busy line.
 *
 * @param void No parameters.
 * @return bool True when ready, false on timeout.
 */
static bool _sx1262_wait(void) {
    /**
     * @brief Declaration of elapsed.
     */
    uint32_t elapsed = 0U;
    sleep_us(25U);
    while (gpio_get(MESHCORE_PIN_BUSY) != 0) {
        sleep_us(10U);
        elapsed += 10U;
        if (elapsed > 100000U) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Send a short command to SX1262.
 *
 * @param command Pointer to the command buffer.
 * @param length Length of the command in bytes.
 * @return bool True if successful, false otherwise.
 */
static bool _sx1262_command(const uint8_t *command, size_t length) {
    /**
     * @brief Declaration of dummy_rx.
     */
    uint8_t dummy_rx[MESH_PACKET_MAX_SIZE + 3U];
    if (command == NULL || length == 0U || length > sizeof(dummy_rx) || !_sx1262_wait()) {
        return false;
    }
    gpio_put(MESHCORE_PIN_NSS, 0);
    sleep_us(1U);
    spi_write_read_blocking(spi1, command, dummy_rx, length);
    sleep_us(1U);
    gpio_put(MESHCORE_PIN_NSS, 1);
    return _sx1262_wait();
}

/**
 * @brief Transfer an SX1262 command while collecting returned bytes.
 *
 * @param buffer Pointer to the command/response buffer.
 * @param length Length of the transfer in bytes.
 * @return bool True if successful, false otherwise.
 */
static bool _sx1262_transfer(uint8_t *buffer, size_t length) {
    /**
     * @brief Declaration of tx_buffer.
     */
    uint8_t tx_buffer[MESH_PACKET_MAX_SIZE + 3U] = {0};
    /**
     * @brief Declaration of rx_buffer.
     */
    uint8_t rx_buffer[MESH_PACKET_MAX_SIZE + 3U] = {0};
    if (buffer == NULL || length == 0U || length > sizeof(tx_buffer) || !_sx1262_wait()) {
        return false;
    }
    memcpy(tx_buffer, buffer, length);
    gpio_put(MESHCORE_PIN_NSS, 0);
    sleep_us(1U);
    spi_write_read_blocking(spi1, tx_buffer, rx_buffer, length);
    sleep_us(1U);
    gpio_put(MESHCORE_PIN_NSS, 1);
    memcpy(buffer, rx_buffer, length);
    return _sx1262_wait();
}

/**
 * @brief Query SX1262 device error flags.
 *
 * @param void No parameters.
 * @return uint16_t 16-bit error flags.
 */
static uint16_t _sx1262_device_errors(void) {
    /**
     * @brief Declaration of command.
     */
    uint8_t command[4U] = {0x17U, 0x00U, 0x00U, 0x00U};
    if (!_sx1262_transfer(command, sizeof(command))) {
        return 0xFFFFU;
    }
    return ((uint16_t)command[2] << 8U) | command[3];
}

/**
 * @brief Query SX1262 status byte.
 *
 * @param void No parameters.
 * @return uint8_t 8-bit status.
 */
static uint8_t _sx1262_status(void) {
    /**
     * @brief Declaration of command.
     */
    uint8_t command[2U] = {0xC0U, 0x00U};
    if (!_sx1262_transfer(command, sizeof(command))) {
        return 0xFFU;
    }
    return command[1];
}

/**
 * @brief Wait until SX1262 finishes transmission or times out.
 *
 * @param timeout_ms Maximum time to wait in milliseconds.
 * @return bool True when TX completed, false on timeout.
 */
static bool _sx1262_wait_tx_done(uint32_t timeout_ms) {
    /**
     * @brief Declaration of elapsed.
     */
    uint32_t elapsed = 0U;
    /**
     * @brief Declaration of irq_status.
     */
    uint8_t irq_status[4U];
    /**
     * @brief Declaration of irq.
     */
    uint16_t irq = 0U;
    while (elapsed < timeout_ms) {
        if (gpio_get(MESHCORE_PIN_DIO1) != 0) {
            printf("[sx1262] tx_done via dio1 pin at %lu ms\n", (unsigned long)elapsed);
            return true;
        }
        irq_status[0] = 0x12U;
        irq_status[1] = 0x00U;
        irq_status[2] = 0x00U;
        irq_status[3] = 0x00U;
        if (_sx1262_transfer(irq_status, sizeof(irq_status))) {
            irq = ((uint16_t)irq_status[2] << 8U) | irq_status[3];
            if (irq != 0xFFFFU && (irq & 0x0001U) != 0U) {
                printf("[sx1262] tx_done at %lu ms dio1=%d irq=0x%04X\n",
                       (unsigned long)elapsed,
                       gpio_get(MESHCORE_PIN_DIO1),
                       (unsigned)irq);
                return true;
            }
            if (irq != 0xFFFFU && (irq & 0x0200U) != 0U) {
                printf("[sx1262] tx_timeout at %lu ms dio1=%d irq=0x%04X\n",
                       (unsigned long)elapsed,
                       gpio_get(MESHCORE_PIN_DIO1),
                       (unsigned)irq);
                return false;
            }
        }
        sleep_ms(10U);
        elapsed += 10U;
    }
    printf("[sx1262] tx host timeout after %lu ms dio1=%d errors=0x%04X\n",
           (unsigned long)elapsed,
           gpio_get(MESHCORE_PIN_DIO1),
           (unsigned)_sx1262_device_errors());
    return false;
}

/**
 * @brief Configure TCXO or XTAL oscillator and calibrate internal blocks.
 *
 * @param void No parameters.
 * @return bool True if oscillator started without error, false otherwise.
 */
static bool _sx1262_start_oscillator(void) {
    /**
     * @brief Declaration of set_rc.
     */
    uint8_t set_rc[] = {0x80U, 0x00U};
    /**
     * @brief Declaration of set_xosc.
     */
    uint8_t set_xosc[] = {0x80U, 0x01U};
    /**
     * @brief Declaration of clear_err.
     */
    uint8_t clear_err[] = {0x07U, 0x00U, 0x00U};
    /**
     * @brief Declaration of calibrate_all.
     */
    uint8_t calibrate_all[] = {0x89U, 0x7FU};
    /**
     * @brief Declaration of calibrate_img.
     */
    uint8_t calibrate_img[] = {0x98U, 0xE1U, 0xE9U};
    /**
     * @brief Declaration of tcxo_voltages.
     */
    uint8_t tcxo_voltages[] = {0x01U, 0x02U, 0x00U, 0x07U};
    /**
     * @brief Declaration of tcxo_cmd.
     */
    uint8_t tcxo_cmd[] = {0x97U, 0x01U, 0x00U, 0x01U, 0x40U};
    /**
     * @brief Declaration of errors.
     */
    uint16_t errors = 0U;
    /**
     * @brief Declaration of i.
     */
    size_t i = 0U;
    for (i = 0U; i < sizeof(tcxo_voltages); i++) {
        tcxo_cmd[1] = tcxo_voltages[i];
        _sx1262_command(set_rc, sizeof(set_rc));
        _sx1262_command(clear_err, sizeof(clear_err));
        _sx1262_command(tcxo_cmd, sizeof(tcxo_cmd));
        sleep_ms(10U);
        _sx1262_wait();
        _sx1262_command(calibrate_all, sizeof(calibrate_all));
        sleep_ms(10U);
        _sx1262_wait();
        _sx1262_command(calibrate_img, sizeof(calibrate_img));
        _sx1262_wait();
        _sx1262_command(clear_err, sizeof(clear_err));
        _sx1262_command(set_xosc, sizeof(set_xosc));
        _sx1262_wait();
        errors = _sx1262_device_errors();
        if ((errors & 0x0020U) == 0U && _sx1262_status() != 0xFFU) {
            printf("[sx1262] tcxo locked volt_code=0x%02X errors=0x%04X status=0x%02X\n",
                   (unsigned)tcxo_voltages[i], (unsigned)errors, (unsigned)_sx1262_status());
            return true;
        }
        printf("[sx1262] tcxo attempt %u (code 0x%02X) errors=0x%04X status=0x%02X\n",
               (unsigned)i, (unsigned)tcxo_voltages[i], (unsigned)errors, (unsigned)_sx1262_status());
    }
    gpio_put(MESHCORE_PIN_RESET, 0);
    sleep_ms(10U);
    gpio_put(MESHCORE_PIN_RESET, 1);
    sleep_ms(10U);
    _sx1262_wait();
    _sx1262_command(set_rc, sizeof(set_rc));
    _sx1262_command(clear_err, sizeof(clear_err));
    _sx1262_command(calibrate_all, sizeof(calibrate_all));
    sleep_ms(10U);
    _sx1262_wait();
    _sx1262_command(calibrate_img, sizeof(calibrate_img));
    _sx1262_wait();
    _sx1262_command(clear_err, sizeof(clear_err));
    _sx1262_command(set_xosc, sizeof(set_xosc));
    _sx1262_wait();
    errors = _sx1262_device_errors();
    printf("[sx1262] xtal fallback errors=0x%04X status=0x%02X\n",
           (unsigned)errors, (unsigned)_sx1262_status());
    return (errors & 0x0020U) == 0U;
}

/**
 * @brief Configure the SX1262 for a basic explicit-header LoRa packet.
 *
 * @param frequency RF frequency in Hertz.
 * @param bandwidth Signal bandwidth in Hertz.
 * @param spreading_factor LoRa spreading factor (SF7-SF12).
 * @param coding_rate LoRa coding rate (5-8).
 * @return bool True if configuration succeeded, false otherwise.
 */
static bool _sx1262_configure(uint32_t frequency,
                              uint32_t bandwidth,
                              uint8_t spreading_factor,
                              uint8_t coding_rate) {
    /**
     * @brief Declaration of rf_frequency.
     */
    uint32_t rf_frequency = (uint32_t)(((uint64_t)frequency << 25U) / 32000000U);
    /**
     * @brief Declaration of packet_type.
     */
    uint8_t packet_type[] = {0x8AU, 0x01U};
    /**
     * @brief Declaration of frequency_command.
     */
    uint8_t frequency_command[] = {0x86U,
                                   (uint8_t)(rf_frequency >> 24U),
                                   (uint8_t)(rf_frequency >> 16U),
                                   (uint8_t)(rf_frequency >> 8U),
                                   (uint8_t)rf_frequency};
    /**
     * @brief Declaration of pa_config.
     */
    uint8_t pa_config[] = {0x95U, 0x04U, 0x07U, 0x00U, 0x01U};
    /**
     * @brief Declaration of modulation.
     */
    uint8_t modulation[] = {
        0x8BU, (uint8_t)spreading_factor, 0x06U, (uint8_t)(coding_rate - 4U), 0x00U};
    /**
     * @brief Declaration of packet.
     */
    uint8_t packet[] = {0x8CU, 0x00U, 0x20U, 0x00U, 0xFFU, 0x01U, 0x00U};
    /**
     * @brief Declaration of power.
     */
    uint8_t power[] = {0x8EU, 0x16U, 0x04U};
    /**
     * @brief Declaration of current_limit.
     */
    uint8_t current_limit[] = {0x0DU, 0x08U, 0xE7U, 0x38U};
    /**
     * @brief Declaration of rx_gain.
     */
    uint8_t rx_gain[] = {0x0DU, 0x08U, 0xACU, 0x96U};
    /**
     * @brief Declaration of buffer.
     */
    uint8_t buffer[] = {0x8FU, 0x00U, 0x00U};
    /**
     * @brief Declaration of irq.
     */
    uint8_t irq[] = {0x08U, 0x02U, 0x63U, 0x00U, 0x03U, 0x00U, 0x00U, 0x00U, 0x00U};
    /**
     * @brief Declaration of regulator.
     */
    uint8_t regulator[] = {0x96U, 0x00U};
    /**
     * @brief Declaration of dio2_switch.
     */
    uint8_t dio2_switch[] = {0x9DU, 0x01U};
    /**
     * @brief Declaration of sync_word.
     */
    uint8_t sync_word[] = {0x0DU, 0x07U, 0x40U, 0x14U, 0x24U};
    /**
     * @brief Declaration of clear_errors.
     */
    uint8_t clear_errors[] = {0x07U, 0x00U, 0x00U};
    if (bandwidth == 7800U) {
        modulation[2] = 0x00U;
    } else if (bandwidth == 10400U) {
        modulation[2] = 0x08U;
    } else if (bandwidth == 15600U) {
        modulation[2] = 0x01U;
    } else if (bandwidth == 20800U) {
        modulation[2] = 0x09U;
    } else if (bandwidth == 31250U) {
        modulation[2] = 0x02U;
    } else if (bandwidth == 41700U) {
        modulation[2] = 0x0AU;
    } else if (bandwidth == 62500U) {
        modulation[2] = 0x03U;
    } else if (bandwidth == 125000U) {
        modulation[2] = 0x04U;
    } else if (bandwidth == 250000U) {
        modulation[2] = 0x05U;
    } else if (bandwidth == 500000U) {
        modulation[2] = 0x06U;
    } else {
        return false;
    }
    return _sx1262_command(regulator, sizeof(regulator)) &&
           _sx1262_command(dio2_switch, sizeof(dio2_switch)) &&
           _sx1262_command(packet_type, sizeof(packet_type)) &&
           _sx1262_command(frequency_command, sizeof(frequency_command)) &&
           _sx1262_command(pa_config, sizeof(pa_config)) &&
           _sx1262_command(modulation, sizeof(modulation)) &&
           _sx1262_command(packet, sizeof(packet)) &&
           _sx1262_command(power, sizeof(power)) &&
           _sx1262_command(current_limit, sizeof(current_limit)) &&
           _sx1262_command(rx_gain, sizeof(rx_gain)) &&
           _sx1262_command(buffer, sizeof(buffer)) &&
           _sx1262_command(sync_word, sizeof(sync_word)) &&
           _sx1262_command(irq, sizeof(irq)) &&
           _sx1262_command(clear_errors, sizeof(clear_errors));
}

/**
 * @brief Initialize SPI, reset SX1262, and configure standby.
 *
 * @param frequency RF frequency in Hertz.
 * @param bandwidth Signal bandwidth in Hertz.
 * @param spreading_factor LoRa spreading factor (SF7-SF12).
 * @param coding_rate LoRa coding rate (5-8).
 * @return bool True if initialization succeeded, false otherwise.
 */
bool sx1262_init(uint32_t frequency,
                 uint32_t bandwidth,
                 uint8_t spreading_factor,
                 uint8_t coding_rate) {
    /**
     * @brief Declaration of set_standby.
     */
    uint8_t set_standby[] = {0x80U, 0x00U};
    /**
     * @brief Declaration of set_regulator.
     */
    uint8_t set_regulator[] = {0x96U, 0x00U};
    /**
     * @brief Declaration of dio2_switch.
     */
    uint8_t dio2_switch[] = {0x9DU, 0x01U};
    /**
     * @brief Declaration of set_rx_mode.
     */
    uint8_t set_rx_mode[] = {0x82U, 0xFFU, 0xFFU, 0xFFU};
    /**
     * @brief Declaration of status.
     */
    uint8_t status = 0U;
    /**
     * @brief Declaration of errors.
     */
    uint16_t errors = 0xFFFFU;
    /**
     * @brief Declaration of initialized.
     */
    bool initialized = false;
    if (!_sx1262_mutex_initialized) {
        mutex_init(&_sx1262_mutex);
        _sx1262_mutex_initialized = true;
    }
    adc_init();
    adc_gpio_init(MESHCORE_PIN_BATTERY_ADC);
    adc_select_input(0U);
    gpio_init(MESHCORE_PIN_ANT_SW);
    gpio_set_dir(MESHCORE_PIN_ANT_SW, GPIO_OUT);
    gpio_put(MESHCORE_PIN_ANT_SW, 1);
    gpio_init(MESHCORE_PIN_NSS);
    gpio_set_dir(MESHCORE_PIN_NSS, GPIO_OUT);
    gpio_put(MESHCORE_PIN_NSS, 1);
    gpio_disable_pulls(MESHCORE_PIN_NSS);
    gpio_init(MESHCORE_PIN_BUSY);
    gpio_set_dir(MESHCORE_PIN_BUSY, GPIO_IN);
    gpio_disable_pulls(MESHCORE_PIN_BUSY);
    gpio_init(MESHCORE_PIN_DIO1);
    gpio_set_dir(MESHCORE_PIN_DIO1, GPIO_IN);
    gpio_disable_pulls(MESHCORE_PIN_DIO1);
    gpio_init(MESHCORE_PIN_RESET);
    gpio_set_dir(MESHCORE_PIN_RESET, GPIO_OUT);
    gpio_put(MESHCORE_PIN_RESET, 0);
    sleep_ms(10U);
    gpio_put(MESHCORE_PIN_RESET, 1);
    sleep_ms(10U);
    spi_init(spi1, 8000000U);
    spi_set_format(spi1, 8U, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(MESHCORE_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(MESHCORE_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(MESHCORE_PIN_MISO, GPIO_FUNC_SPI);
    gpio_disable_pulls(MESHCORE_PIN_SCK);
    gpio_disable_pulls(MESHCORE_PIN_MOSI);
    gpio_disable_pulls(MESHCORE_PIN_MISO);
    _sx1262_wait();
    status = _sx1262_status();
    printf("[sx1262] post-reset status=0x%02X busy=%d dio1=%d\n",
           (unsigned)status, gpio_get(MESHCORE_PIN_BUSY), gpio_get(MESHCORE_PIN_DIO1));
    mutex_enter_blocking(&_sx1262_mutex);
    initialized = _sx1262_command(set_standby, sizeof(set_standby)) &&
                  _sx1262_command(set_regulator, sizeof(set_regulator)) &&
                  _sx1262_start_oscillator() &&
                  _sx1262_command(dio2_switch, sizeof(dio2_switch)) &&
                  _sx1262_configure(frequency, bandwidth, spreading_factor, coding_rate) &&
                  _sx1262_command(set_rx_mode, sizeof(set_rx_mode));
    _sx1262_wait();
    status = _sx1262_status();
    errors = _sx1262_device_errors();
    _sx1262_ready = initialized && (status != 0xFFU) && (status != 0x00U);
    mutex_exit(&_sx1262_mutex);
    printf("[sx1262] init=%s ready=%s frequency=%lu bandwidth=%lu sf=%u cr=%u dio1=%d busy=%d status=0x%02X errors=0x%04X\n",
           initialized ? "ok" : "failed",
           _sx1262_ready ? "true" : "false",
           (unsigned long)frequency,
           (unsigned long)bandwidth,
           (unsigned)spreading_factor,
           (unsigned)coding_rate,
           gpio_get(MESHCORE_PIN_DIO1),
           gpio_get(MESHCORE_PIN_BUSY),
           (unsigned)status,
           (unsigned)errors);
    return _sx1262_ready;
}

/**
 * @brief Apply new LoRa modulation parameters to the active SX1262.
 *
 * @param frequency RF frequency in Hertz.
 * @param bandwidth Signal bandwidth in Hertz.
 * @param spreading_factor LoRa spreading factor (SF7-SF12).
 * @param coding_rate LoRa coding rate (5-8).
 * @return bool True if parameters were applied, false otherwise.
 */
bool sx1262_set_params(uint32_t frequency,
                       uint32_t bandwidth,
                       uint8_t spreading_factor,
                       uint8_t coding_rate) {
    /**
     * @brief Declaration of set_rx_mode.
     */
    uint8_t set_rx_mode[] = {0x82U, 0xFFU, 0xFFU, 0xFFU};
    /**
     * @brief Declaration of result.
     */
    bool result = false;
    if (!mutex_enter_timeout_ms(&_sx1262_mutex, 1000U)) {
        return false;
    }
    result = _sx1262_configure(frequency, bandwidth, spreading_factor, coding_rate) &&
             _sx1262_command(set_rx_mode, sizeof(set_rx_mode));
    mutex_exit(&_sx1262_mutex);
    return result;
}

/**
 * @brief Transmit one raw frame using the SX1262 buffer command.
 *
 * @param data Pointer to the buffer containing data to transmit.
 * @param length Length of data in bytes.
 * @return bool True if transmission was successful, false otherwise.
 */
bool sx1262_transmit(const uint8_t *data, size_t length) {
    /**
     * @brief Declaration of command.
     */
    uint8_t command[2U + MESH_PACKET_MAX_SIZE] = {0};
    /**
     * @brief Declaration of packet_params.
     */
    uint8_t packet_params[] = {0x8CU, 0x00U, 0x20U, 0x00U, (uint8_t)length, 0x01U, 0x00U};
    /**
     * @brief Declaration of rx_packet.
     */
    uint8_t rx_packet[] = {0x8CU, 0x00U, 0x20U, 0x00U, 0xFFU, 0x01U, 0x00U};
    /**
     * @brief Declaration of transmit.
     */
    uint8_t transmit[] = {0x83U, 0x00U, 0x00U, 0x00U};
    /**
     * @brief Declaration of clear_irq.
     */
    uint8_t clear_irq[] = {0x02U, 0xFFU, 0xFFU};
    /**
     * @brief Declaration of receive.
     */
    uint8_t receive[] = {0x82U, 0xFFU, 0xFFU, 0xFFU};
    /**
     * @brief Declaration of status.
     */
    uint8_t status = 0U;
    /**
     * @brief Declaration of errors.
     */
    uint16_t errors = 0xFFFFU;
    /**
     * @brief Declaration of sent.
     */
    bool sent = false;
    if (!_sx1262_ready || data == NULL || length == 0U || length > MESH_PACKET_MAX_SIZE) {
        return false;
    }
    if (!mutex_enter_timeout_ms(&_sx1262_mutex, 3000U)) {
        return false;
    }
    command[0] = 0x0EU;
    command[1] = 0x00U;
    memcpy(&command[2], data, length);
    _sx1262_command(packet_params, sizeof(packet_params));
    _sx1262_command(command, length + 2U);
    _sx1262_command(clear_irq, sizeof(clear_irq));
    sent = _sx1262_command(transmit, sizeof(transmit));
    if (sent) {
        sent = _sx1262_wait_tx_done(2000U);
    }
    _sx1262_command(clear_irq, sizeof(clear_irq));
    _sx1262_command(rx_packet, sizeof(rx_packet));
    _sx1262_command(receive, sizeof(receive));
    status = _sx1262_status();
    errors = _sx1262_device_errors();
    mutex_exit(&_sx1262_mutex);
    printf("[sx1262] tx=%s length=%u status=0x%02X errors=0x%04X\n",
           sent ? "ok" : "failed",
           (unsigned)length,
           (unsigned)status,
           (unsigned)errors);
    return sent;
}

/**
 * @brief Read one completed SX1262 receive frame and restart reception.
 *
 * @param data Destination buffer to copy the received payload.
 * @param capacity Maximum capacity of the destination buffer.
 * @return size_t Number of bytes received, or 0 on error/no packet.
 */
size_t sx1262_receive(uint8_t *data, size_t capacity) {
    /**
     * @brief Declaration of irq_status.
     */
    uint8_t irq_status[4U] = {0x12U, 0x00U, 0x00U, 0x00U};
    /**
     * @brief Declaration of status.
     */
    uint8_t status[4U] = {0x13U, 0x00U, 0x00U, 0x00U};
    /**
     * @brief Declaration of buffer.
     */
    uint8_t buffer[3U + MESH_PACKET_MAX_SIZE] = {0x1EU, 0x00U, 0x00U};
    /**
     * @brief Declaration of clear_irq.
     */
    uint8_t clear_irq[] = {0x02U, 0xFFU, 0xFFU};
    /**
     * @brief Declaration of receive.
     */
    uint8_t receive[] = {0x82U, 0xFFU, 0xFFU, 0xFFU};
    /**
     * @brief Declaration of packet_status.
     */
    uint8_t packet_status[5U] = {0x14U, 0x00U, 0x00U, 0x00U, 0x00U};
    /**
     * @brief Declaration of length.
     */
    size_t length = 0U;
    /**
     * @brief Declaration of irq.
     */
    uint16_t irq = 0U;
    if (!_sx1262_ready || data == NULL || capacity == 0U) {
        return 0U;
    }
    if (gpio_get(MESHCORE_PIN_DIO1) == 0) {
        return 0U;
    }
    if (!mutex_try_enter(&_sx1262_mutex, NULL)) {
        return 0U;
    }
    if (!_sx1262_transfer(irq_status, sizeof(irq_status))) {
        mutex_exit(&_sx1262_mutex);
        return 0U;
    }
    irq = ((uint16_t)irq_status[2] << 8U) | irq_status[3];
    if (irq != 0U) {
        _sx1262_command(clear_irq, sizeof(clear_irq));
    }
    if ((irq & 0x0002U) == 0U) {
        if ((irq & 0x0260U) != 0U) {
            _sx1262_command(receive, sizeof(receive));
        }
        mutex_exit(&_sx1262_mutex);
        return 0U;
    }
    if ((irq & 0x0040U) != 0U) {
        _sx1262_command(receive, sizeof(receive));
        mutex_exit(&_sx1262_mutex);
        return 0U;
    }
    if (!_sx1262_transfer(status, sizeof(status))) {
        _sx1262_command(receive, sizeof(receive));
        mutex_exit(&_sx1262_mutex);
        return 0U;
    }
    length = status[2];
    if (length == 0U || length > MESH_PACKET_MAX_SIZE || length > capacity) {
        _sx1262_command(receive, sizeof(receive));
        mutex_exit(&_sx1262_mutex);
        return 0U;
    }
    buffer[1] = status[3];
    if (!_sx1262_transfer(buffer, length + 3U)) {
        _sx1262_command(receive, sizeof(receive));
        mutex_exit(&_sx1262_mutex);
        return 0U;
    }
    memcpy(data, &buffer[3], length);
    if (_sx1262_transfer(packet_status, sizeof(packet_status))) {
        _last_rssi = packet_status[2] > 0U ? (int8_t)(-(int16_t)packet_status[2] / 2) : (int8_t)-60;
        _last_snr_x4 = (int8_t)packet_status[3];
    }
    _sx1262_command(receive, sizeof(receive));
    mutex_exit(&_sx1262_mutex);
    printf("[sx1262] rx=ok length=%u rssi=%d snr=%d\n",
           (unsigned)length,
           (int)_last_rssi,
           (int)(_last_snr_x4 / 4));
    return length;
}

/**
 * @brief Return the SNR times four of the last received LoRa frame.
 *
 * @param void No parameters.
 * @return int8_t SNR times four as a signed 8-bit integer.
 */
int8_t sx1262_last_snr_x4(void) {
    return _last_snr_x4;
}

/**
 * @brief Return the RSSI in dBm of the last received LoRa frame.
 *
 * @param void No parameters.
 * @return int8_t RSSI in dBm as a signed 8-bit integer.
 */
int8_t sx1262_last_rssi(void) {
    return _last_rssi;
}
