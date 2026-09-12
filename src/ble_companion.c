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
// File:    ble_companion.c
// Desc:    Implements the MeshCore BLE companion transport via BTstack.
// Created: 2026

#include "ble_companion.h"

#include "ble/att_server.h"
#include "ble/gatt-service/nordic_spp_service_server.h"
#include "ble/le_device_db.h"
#include "ble/sm.h"
#include "btstack.h"
#include "btstack_event.h"
#include "companion.h"
#include "config.h"
#include "meshcore.h"
#include "node_state.h"
#include "storage.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Maximum BLE companion response frame size in bytes.
 */
#define BLE_COMPANION_MAX_FRAME 172U

/**
 * @brief Active BLE connection handle.
 */
static hci_con_handle_t _con_handle = HCI_CON_HANDLE_INVALID;

/**
 * @brief Advertisement data buffer.
 */
static uint8_t _adv_data[31U];

/**
 * @brief Scan response data buffer.
 */
static uint8_t _scan_data[31U];

/**
 * @brief Length of advertisement data buffer.
 */
static uint8_t _adv_data_len = 0U;

/**
 * @brief Length of scan response data buffer.
 */
static uint8_t _scan_data_len = 0U;

/**
 * @brief Outgoing response buffer for chunked transmission.
 */
static uint8_t _tx_chunk_buffer[BLE_COMPANION_MAX_FRAME];

/**
 * @brief Total length of current outgoing response in bytes.
 */
static uint16_t _tx_chunk_len = 0U;

/**
 * @brief Offset of bytes already transmitted in current outgoing response.
 */
static uint16_t _tx_chunk_offset = 0U;

/**
 * @brief BTstack can-send-now request callback registration.
 */
static btstack_context_callback_registration_t _send_request;

/**
 * @brief Flag indicating whether a can-send-now request is pending.
 */
static bool _send_request_pending = false;

/**
 * @brief BTstack SM packet callback registration.
 */
static btstack_packet_callback_registration_t _sm_callback_registration;

/**
 * @brief BTstack HCI packet callback registration.
 */
static btstack_packet_callback_registration_t _hci_callback_registration;

/**
 * @brief Callback when BTstack is ready to transmit the next notification.
 *
 * @param context User context pointer (unused).
 * @return void
 */
static void _can_send_now_callback(void *context) {
    (void)context;
    _send_request_pending = false;
    ble_companion_flush_pending();
}

/**
 * @brief ATT packet handler for MTU exchange and notification flow control.
 *
 * @param packet_type BTstack packet type.
 * @param channel Channel ID.
 * @param packet Pointer to packet data.
 * @param size Packet size in bytes.
 * @return void
 */
static void _att_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_CONNECTED:
            printf("[ble_companion] ATT connected handle=0x%04x\n",
                   (unsigned)att_event_connected_get_handle(packet));
            break;
        case ATT_EVENT_DISCONNECTED:
            printf("[ble_companion] ATT disconnected handle=0x%04x\n",
                   (unsigned)att_event_disconnected_get_handle(packet));
            break;
        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            printf("[ble_companion] ATT MTU exchange complete handle=0x%04x mtu=%u\n",
                   (unsigned)att_event_mtu_exchange_complete_get_handle(packet),
                   (unsigned)att_event_mtu_exchange_complete_get_MTU(packet));
            ble_companion_flush_pending();
            break;
        case ATT_EVENT_CAN_SEND_NOW:
            _send_request_pending = false;
            ble_companion_flush_pending();
            break;
        default:
            break;
    }
}

/**
 * @brief Security manager packet handler for pairing and authentication events.
 *
 * @param packet_type BTstack packet type.
 * @param channel Channel ID.
 * @param packet Pointer to packet data.
 * @param size Packet size in bytes.
 * @return void
 */
static void _sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_META_GAP:
            if (hci_event_gap_meta_get_subevent_code(packet) == GAP_SUBEVENT_LE_CONNECTION_COMPLETE) {
                /**
                 * @brief Declaration of handle.
                 */
                hci_con_handle_t handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
                _con_handle = handle;
                printf("[ble_companion] GAP connected handle=0x%04x\n", (unsigned)handle);
                sm_request_pairing(handle);
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("[ble_companion] HCI disconnection complete\n");
            _con_handle = HCI_CON_HANDLE_INVALID;
            _send_request_pending = false;
            _tx_chunk_len = 0U;
            _tx_chunk_offset = 0U;
            ble_companion_start();
            break;
        case SM_EVENT_JUST_WORKS_REQUEST: {
            /**
             * @brief Declaration of handle.
             */
            hci_con_handle_t handle = sm_event_just_works_request_get_handle(packet);
            printf("[ble_companion] Just Works requested, declining for passkey authentication handle=0x%04x\n",
                   (unsigned)handle);
            sm_bonding_decline(handle);
            break;
        }
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST: {
            /**
             * @brief Declaration of handle.
             */
            hci_con_handle_t handle = sm_event_numeric_comparison_request_get_handle(packet);
            printf("[ble_companion] Numeric comparison requested, declining for passkey authentication handle=0x%04x\n",
                   (unsigned)handle);
            sm_bonding_decline(handle);
            break;
        }
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            printf("[ble_companion] Fixed pairing passkey: %06lu\n",
                   (unsigned long)sm_event_passkey_display_number_get_passkey(packet));
            break;
        case SM_EVENT_PAIRING_COMPLETE:
            printf("[ble_companion] Pairing complete status=0x%02x\n",
                   (unsigned)sm_event_pairing_complete_get_status(packet));
            break;
        case SM_EVENT_REENCRYPTION_STARTED:
            printf("[ble_companion] Re-encryption started\n");
            break;
        case SM_EVENT_REENCRYPTION_COMPLETE: {
            /**
             * @brief Declaration of status.
             */
            uint8_t status = sm_event_reencryption_complete_get_status(packet);
            printf("[ble_companion] Re-encryption complete status=0x%02x\n", (unsigned)status);
            if (status == ERROR_CODE_PIN_OR_KEY_MISSING) {
                /**
                 * @brief Declaration of addr.
                 */
                bd_addr_t addr;
                sm_event_reencryption_complete_get_address(packet, addr);
                /**
                 * @brief Declaration of addr_type.
                 */
                bd_addr_type_t addr_type = (bd_addr_type_t)sm_event_reencryption_started_get_addr_type(packet);
                gap_delete_bonding(addr_type, addr);
                printf("[ble_companion] Deleted stale bonding for re-pairing\n");
            }
            break;
        }
        default:
            break;
    }
}

/**
 * @brief Nordic SPP service packet handler for connection lifecycle and incoming data.
 *
 * @param packet_type BTstack packet type.
 * @param channel Channel ID.
 * @param packet Pointer to packet data.
 * @param size Packet size in bytes.
 * @return void
 */
static void _nordic_spp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            if (hci_event_packet_get_type(packet) == HCI_EVENT_GATTSERVICE_META) {
                switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
                    case GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED:
                        _con_handle = gattservice_subevent_spp_service_connected_get_con_handle(packet);
                        printf("[ble_companion] Nordic SPP connected handle=0x%04x\n", (unsigned)_con_handle);
                        ble_companion_flush_pending();
                        break;
                    case GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED:
                        printf("[ble_companion] Nordic SPP disconnected\n");
                        _con_handle = HCI_CON_HANDLE_INVALID;
                        _send_request_pending = false;
                        _tx_chunk_len = 0U;
                        _tx_chunk_offset = 0U;
                        ble_companion_start();
                        break;
                    default:
                        break;
                }
            }
            break;
        case RFCOMM_DATA_PACKET:
            if (_con_handle == HCI_CON_HANDLE_INVALID) {
                _con_handle = (hci_con_handle_t)channel;
            }
            printf("[ble_companion] RX data handle=0x%04x len=%u cmd=0x%02x\n",
                   (unsigned)_con_handle, (unsigned)size, size > 0U ? packet[0] : 0U);
            companion_receive(packet, size);
            ble_companion_flush_pending();
            break;
        default:
            break;
    }
}

/**
 * @brief Build advertisement and scan response payloads matching the active node name.
 *
 * @param void No parameters.
 * @return void
 */
static void _build_adv_data(void) {
    /**
     * @brief Declaration of name.
     */
    const char *name = node_state_get_name();
    /**
     * @brief Declaration of name_len.
     */
    size_t name_len = strlen(name);
    /**
     * @brief Declaration of adv_len.
     */
    size_t adv_len = 0U;
    /**
     * @brief Declaration of nus_uuid.
     */
    static const uint8_t nus_uuid[16U] = {
        0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
        0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e
    };
    _adv_data[0] = 2U;
    _adv_data[1] = BLUETOOTH_DATA_TYPE_FLAGS;
    _adv_data[2] = 0x06U;
    _adv_data[3] = 17U;
    _adv_data[4] = BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS;
    memcpy(&_adv_data[5], nus_uuid, 16U);
    adv_len = 21U;
    if (name_len > 8U) {
        name_len = 8U;
    }
    if (name_len > 0U) {
        _adv_data[adv_len] = (uint8_t)(name_len + 1U);
        _adv_data[adv_len + 1U] = BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME;
        memcpy(&_adv_data[adv_len + 2U], name, name_len);
        adv_len += name_len + 2U;
    }
    _adv_data_len = (uint8_t)adv_len;
    name_len = strlen(name);
    if (name_len > 29U) {
        name_len = 29U;
    }
    _scan_data[0] = (uint8_t)(name_len + 1U);
    _scan_data[1] = BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME;
    memcpy(&_scan_data[2], name, name_len);
    _scan_data_len = (uint8_t)(name_len + 2U);
}

/**
 * @brief Handle dynamic GATT attribute reads, serving the live node name.
 *
 * @param con_handle Connection handle of the requesting client.
 * @param att_handle Attribute handle being read.
 * @param offset Byte offset for read blob requests.
 * @param buffer Output buffer for the attribute value.
 * @param buffer_size Capacity of the output buffer.
 * @return uint16_t Length of the requested attribute value.
 */
static uint16_t _att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    (void)con_handle;
    if (att_handle == ATT_CHARACTERISTIC_GAP_DEVICE_NAME_01_VALUE_HANDLE) {
        return att_read_callback_handle_blob((const uint8_t *)node_state_get_name(),
                                             (uint16_t)strlen(node_state_get_name()),
                                             offset, buffer, buffer_size);
    }
    return 0;
}

/**
 * @brief Initialize MeshCore BLE services and security configuration.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_init(void) {
    /**
     * @brief Declaration of count.
     */
    int count = 0;
    /**
     * @brief Declaration of b_idx.
     */
    int b_idx = 0;
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_DISPLAY_ONLY);
    sm_set_authentication_requirements(SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING);
    sm_set_accepted_stk_generation_methods(SM_STK_GENERATION_METHOD_PASSKEY);
    sm_use_fixed_passkey_in_display_role(MESHCORE_BLE_PIN);
    if (storage_claim_ble_bond_reset()) {
        count = le_device_db_count();
        for (b_idx = count - 1; b_idx >= 0; b_idx--) {
            le_device_db_remove(b_idx);
        }
        printf("[ble_companion] Cleared stale BLE bonds for pairing reset\n");
    }
    _hci_callback_registration.callback = &_sm_packet_handler;
    hci_add_event_handler(&_hci_callback_registration);
    _sm_callback_registration.callback = &_sm_packet_handler;
    sm_add_event_handler(&_sm_callback_registration);
    att_server_init(profile_data, &_att_read_callback, NULL);
    att_server_register_packet_handler(&_att_packet_handler);
    nordic_spp_service_server_init(&_nordic_spp_packet_handler);
}

/**
 * @brief Start MeshCore BLE advertising and host processing.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_start(void) {
    /**
     * @brief Declaration of null_addr.
     */
    bd_addr_t null_addr = {0};
    _build_adv_data();
    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(_adv_data_len, _adv_data);
    gap_scan_response_set_data(_scan_data_len, _scan_data);
    gap_advertisements_enable(1);
    hci_power_control(HCI_POWER_ON);
    printf("[ble_companion] BLE advertising started as %s\n", node_state_get_name());
}

/**
 * @brief Flush queued companion responses to the connected BLE client.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_flush_pending(void) {
    if (_con_handle == HCI_CON_HANDLE_INVALID) {
        return;
    }
    while (_con_handle != HCI_CON_HANDLE_INVALID) {
        if (_tx_chunk_offset >= _tx_chunk_len) {
            _tx_chunk_len = (uint16_t)companion_next_response(_tx_chunk_buffer, sizeof(_tx_chunk_buffer));
            _tx_chunk_offset = 0U;
            if (_tx_chunk_len == 0U) {
                break;
            }
        }
        /**
         * @brief Declaration of mtu.
         */
        uint16_t mtu = att_server_get_mtu(_con_handle);
        /**
         * @brief Declaration of max_payload.
         */
        uint16_t max_payload = (mtu >= 3U) ? (mtu - 3U) : 20U;
        if (max_payload < 20U) {
            max_payload = 20U;
        }
        /**
         * @brief Declaration of remaining.
         */
        uint16_t remaining = _tx_chunk_len - _tx_chunk_offset;
        /**
         * @brief Declaration of chunk_size.
         */
        uint16_t chunk_size = (remaining > max_payload) ? max_payload : remaining;
        /**
         * @brief Declaration of status.
         */
        int status = nordic_spp_service_server_send(_con_handle, &_tx_chunk_buffer[_tx_chunk_offset], chunk_size);
        printf("[ble_companion] TX handle=0x%04x offset=%u/%u size=%u mtu=%u status=%d\n",
               (unsigned)_con_handle, (unsigned)_tx_chunk_offset, (unsigned)_tx_chunk_len,
               (unsigned)chunk_size, (unsigned)mtu, status);
        if (status == ERROR_CODE_SUCCESS) {
            _tx_chunk_offset += chunk_size;
            if (_tx_chunk_offset < _tx_chunk_len) {
                if (!_send_request_pending) {
                    _send_request_pending = true;
                    _send_request.callback = &_can_send_now_callback;
                    _send_request.context = NULL;
                    nordic_spp_service_server_request_can_send_now(&_send_request, _con_handle);
                }
                break;
            }
        } else if (status == BTSTACK_ACL_BUFFERS_FULL) {
            if (!_send_request_pending) {
                _send_request_pending = true;
                _send_request.callback = &_can_send_now_callback;
                _send_request.context = NULL;
                nordic_spp_service_server_request_can_send_now(&_send_request, _con_handle);
            }
            break;
        } else {
            printf("[ble_companion] TX error %d, discarding frame\n", status);
            _tx_chunk_len = 0U;
            _tx_chunk_offset = 0U;
            break;
        }
    }
}

/**
 * @brief Update the advertised and GAP BLE device name.
 *
 * @param void No parameters.
 * @return void
 */
void ble_companion_update_device_name(void) {
    _build_adv_data();
    gap_advertisements_set_data(_adv_data_len, _adv_data);
    gap_scan_response_set_data(_scan_data_len, _scan_data);
}
