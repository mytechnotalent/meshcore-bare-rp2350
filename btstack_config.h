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
// File:    btstack_config.h
// Desc:    BTstack configuration for the RP2350 MeshCore companion peripheral.
// Created: 2026

#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

/**
 * @brief Enable BLE peripheral role.
 */
#define ENABLE_LE_PERIPHERAL

/**
 * @brief Enable software AES128.
 */
#define ENABLE_SOFTWARE_AES128

/**
 * @brief Enable logging info messages.
 */
#define ENABLE_LOG_INFO

/**
 * @brief Enable logging error messages.
 */
#define ENABLE_LOG_ERROR

/**
 * @brief Enable hexdump output via printf.
 */
#define ENABLE_PRINTF_HEXDUMP

/**
 * @brief HCI outgoing pre-buffer size.
 */
#define HCI_OUTGOING_PRE_BUFFER_SIZE 4

/**
 * @brief HCI ACL payload buffer size.
 */
#define HCI_ACL_PAYLOAD_SIZE (1691 + 4)

/**
 * @brief HCI ACL chunk alignment.
 */
#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4

/**
 * @brief Maximum concurrent HCI connections.
 */
#define MAX_NR_HCI_CONNECTIONS 1

/**
 * @brief Maximum concurrent L2CAP channels.
 */
#define MAX_NR_L2CAP_CHANNELS 2

/**
 * @brief Maximum concurrent L2CAP services.
 */
#define MAX_NR_L2CAP_SERVICES 2

/**
 * @brief Maximum Security Manager lookup entries.
 */
#define MAX_NR_SM_LOOKUP_ENTRIES 3

/**
 * @brief Maximum whitelist entries.
 */
#define MAX_NR_WHITELIST_ENTRIES 1

/**
 * @brief Maximum bonded LE device entries.
 */
#define MAX_NR_LE_DEVICE_DB_ENTRIES 4

/**
 * @brief Number of persistent NVM device DB entries.
 */
#define NVM_NUM_DEVICE_DB_ENTRIES 16

/**
 * @brief Number of persistent NVM link key entries.
 */
#define NVM_NUM_LINK_KEYS 16

/**
 * @brief Maximum controller ACL buffers for CYW43 shared bus.
 */
#define MAX_NR_CONTROLLER_ACL_BUFFERS 3

/**
 * @brief Maximum controller SCO packets for CYW43 shared bus.
 */
#define MAX_NR_CONTROLLER_SCO_PACKETS 0

/**
 * @brief Enable host flow control for HCI controller.
 */
#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL

/**
 * @brief HCI host ACL packet length.
 */
#define HCI_HOST_ACL_PACKET_LEN 1024

/**
 * @brief HCI host ACL packet count.
 */
#define HCI_HOST_ACL_PACKET_NUM 3

/**
 * @brief HCI host SCO packet length.
 */
#define HCI_HOST_SCO_PACKET_LEN 0

/**
 * @brief HCI host SCO packet count.
 */
#define HCI_HOST_SCO_PACKET_NUM 0

/**
 * @brief Maximum ATT database size in bytes.
 */
#define MAX_ATT_DB_SIZE 512

/**
 * @brief Platform provides embedded milliseconds timer.
 */
#define HAVE_EMBEDDED_TIME_MS

/**
 * @brief Map assertion macro to platform assert.
 */
#define HAVE_ASSERT

/**
 * @brief HCI reset timeout in milliseconds.
 */
#define HCI_RESET_RESEND_TIMEOUT_MS 1000

#endif // BTSTACK_CONFIG_H
