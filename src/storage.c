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
// File:    storage.c
// Desc:    Persists portable MeshCore node state in RP2350 on-board flash memory.
// Created: 2026

#include "storage.h"

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "mesh_crypto.h"
#include "node_state.h"
#include "pico/rand.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Storage header magic word ("MCS1").
 */
#define STORAGE_MAGIC 0x3153434DU

/**
 * @brief Storage layout version.
 */
#define STORAGE_VERSION 2U

/**
 * @brief Maximum serialized node state size in bytes.
 */
#define STORAGE_STATE_SIZE 12288U

/**
 * @brief Flash sector offset for node state storage (last 64KB of 4MB flash).
 */
#ifndef STORAGE_FLASH_OFFSET
#define STORAGE_FLASH_OFFSET ((4U * 1024U * 1024U) - (64U * 1024U))
#endif

/**
 * @brief Number of flash sectors allocated for node state storage (16KB).
 */
#define STORAGE_FLASH_SECTORS 4U

/**
 * @brief Flash storage record structure.
 */
typedef struct {
    /**
     * @brief Storage magic word for validation.
     */
    uint32_t magic;
    /**
     * @brief Storage version.
     */
    uint32_t version;
    /**
     * @brief BLE bond reset claimed marker.
     */
    uint8_t ble_reset_claimed;
    /**
     * @brief Dedicated node name null-terminated string.
     */
    char node_name[32U];
    /**
     * @brief Flood scope name null-terminated string.
     */
    uint8_t scope_name[32U];
    /**
     * @brief Flood scope key bytes.
     */
    uint8_t scope_key[16U];
    /**
     * @brief Length of valid identity data.
     */
    uint32_t identity_length;
    /**
     * @brief Exported identity buffer.
     */
    uint8_t identity_data[128U];
    /**
     * @brief Length of valid serialized state data.
     */
    uint32_t state_length;
    /**
     * @brief Checksum over record fields preceding checksum.
     */
    uint32_t checksum;
    /**
     * @brief Serialized node state payload.
     */
    uint8_t state_data[STORAGE_STATE_SIZE];
} StorageRecord;

/**
 * @brief Whether storage is initialized.
 */
static bool _storage_ready = false;

/**
 * @brief Persistent local identity.
 */
static MeshIdentity _identity;

/**
 * @brief In-memory working copy of the persistent storage record.
 */
static StorageRecord _storage_record;

/**
 * @brief Calculate Fletcher32 checksum of data buffer.
 *
 * @param data Pointer to input data.
 * @param length Length of data in bytes.
 * @return uint32_t Calculated checksum.
 */
static uint32_t _storage_checksum(const uint8_t *data, size_t length) {
    /**
     * @brief Declaration of sum1.
     */
    uint32_t sum1 = 0xffffU;
    /**
     * @brief Declaration of sum2.
     */
    uint32_t sum2 = 0xffffU;
    /**
     * @brief Declaration of i.
     */
    size_t i = 0U;
    for (i = 0U; i < length; i++) {
        sum1 = (sum1 + data[i]) % 65535U;
        sum2 = (sum2 + sum1) % 65535U;
    }
    return (sum2 << 16U) | sum1;
}

/**
 * @brief Write the in-memory storage record to flash.
 *
 * @param void No parameters.
 * @return bool true if successfully committed to flash, false otherwise.
 */
static bool _storage_write_flash(void) {
    /**
     * @brief Declaration of program_len.
     */
    size_t program_len = (sizeof(StorageRecord) + (FLASH_PAGE_SIZE - 1U)) & ~(FLASH_PAGE_SIZE - 1U);
    /**
     * @brief Declaration of erase_len.
     */
    size_t erase_len = STORAGE_FLASH_SECTORS * FLASH_SECTOR_SIZE;
    /**
     * @brief Declaration of ints.
     */
    uint32_t ints = 0U;
    _storage_record.magic = STORAGE_MAGIC;
    _storage_record.version = STORAGE_VERSION;
    _storage_record.checksum = _storage_checksum(
        (const uint8_t *)&_storage_record,
        offsetof(StorageRecord, checksum));
    ints = save_and_disable_interrupts();
    flash_range_erase(STORAGE_FLASH_OFFSET, erase_len);
    flash_range_program(STORAGE_FLASH_OFFSET, (const uint8_t *)&_storage_record, program_len);
    restore_interrupts(ints);
    return true;
}

/**
 * @brief Initialize storage and load saved node state.
 *
 * @param void No parameters.
 * @return bool true when storage is ready and loaded, false on failure.
 */
bool storage_init(void) {
    /**
     * @brief Declaration of flash_ptr.
     */
    const StorageRecord *flash_ptr = (const StorageRecord *)(XIP_BASE + STORAGE_FLASH_OFFSET);
    /**
     * @brief Declaration of valid.
     */
    bool valid = false;
    /**
     * @brief Declaration of seed.
     */
    uint8_t seed[32U] = {0};
    /**
     * @brief Declaration of board_id.
     */
    pico_unique_board_id_t board_id;
    /**
     * @brief Declaration of r.
     */
    uint32_t r = 0U;
    /**
     * @brief Declaration of i.
     */
    size_t i = 0U;
    mesh_identity_init(&_identity);
    memset(&_storage_record, 0, sizeof(_storage_record));
    if (flash_ptr->magic == STORAGE_MAGIC && flash_ptr->version == STORAGE_VERSION) {
        /**
         * @brief Declaration of expected_checksum.
         */
        uint32_t expected_checksum = _storage_checksum(
            (const uint8_t *)flash_ptr,
            offsetof(StorageRecord, checksum));
        if (flash_ptr->checksum == expected_checksum) {
            memcpy(&_storage_record, flash_ptr, sizeof(StorageRecord));
            valid = true;
        }
    }
    if (valid && _storage_record.identity_length > 0U) {
        mesh_identity_import(&_identity, _storage_record.identity_data, _storage_record.identity_length);
    }
    if (!_identity.valid) {
        pico_get_unique_board_id(&board_id);
        for (i = 0U; i < sizeof(seed); i += sizeof(uint32_t)) {
            r = get_rand_32();
            memcpy(&seed[i], &r, sizeof(uint32_t));
        }
        for (i = 0U; i < sizeof(board_id.id) && i < sizeof(seed); i++) {
            seed[i] ^= board_id.id[i];
        }
        mesh_identity_from_seed(&_identity, seed);
        _storage_record.identity_length = 0U;
        mesh_identity_export(
            &_identity,
            _storage_record.identity_data,
            sizeof(_storage_record.identity_data),
            (size_t *)&_storage_record.identity_length);
        _storage_write_flash();
    }
    if (valid && _storage_record.state_length > 0U && _storage_record.state_length <= STORAGE_STATE_SIZE) {
        node_state_import(_storage_record.state_data, _storage_record.state_length);
    }
    if (valid && _storage_record.node_name[0] != '\0') {
        node_state_set_name((const uint8_t *)_storage_record.node_name, strlen(_storage_record.node_name));
        node_state_clear_dirty();
        printf("[storage] Restored device name: %s\n", _storage_record.node_name);
    }
    if (valid && _storage_record.scope_name[0] != '\0') {
        /**
         * @brief Declaration of scope_name_len.
         */
        size_t scope_name_len = strnlen((const char *)_storage_record.scope_name, sizeof(_storage_record.scope_name));
        node_state_set_default_scope(_storage_record.scope_name, scope_name_len, _storage_record.scope_key);
        node_state_clear_dirty();
    }
    _storage_ready = true;
    return true;
}

/**
 * @brief Save node state to persistent flash when it has changed.
 *
 * @param void No parameters.
 * @return bool true on success or if state was clean, false on write error.
 */
bool storage_save_if_dirty(void) {
    /**
     * @brief Declaration of length.
     */
    size_t length = 0U;
    /**
     * @brief Declaration of current_name.
     */
    const char *current_name = NULL;
    if (!_storage_ready || !node_state_is_dirty()) {
        return true;
    }
    current_name = node_state_get_name();
    if (current_name != NULL && current_name[0] != '\0') {
        strncpy(_storage_record.node_name, current_name, sizeof(_storage_record.node_name) - 1U);
        _storage_record.node_name[sizeof(_storage_record.node_name) - 1U] = '\0';
    }
    node_state_get_default_scope(_storage_record.scope_name, _storage_record.scope_key);
    if (node_state_export(_storage_record.state_data, STORAGE_STATE_SIZE, &length)) {
        _storage_record.state_length = (uint32_t)length;
    }
    _storage_write_flash();
    node_state_clear_dirty();
    return true;
}

/**
 * @brief Save the dedicated node name directly to persistent flash.
 *
 * @param name Pointer to null-terminated name string.
 * @return bool true if successfully saved and committed, false otherwise.
 */
bool storage_save_node_name(const char *name) {
    if (!_storage_ready || name == NULL || name[0] == '\0') {
        return false;
    }
    strncpy(_storage_record.node_name, name, sizeof(_storage_record.node_name) - 1U);
    _storage_record.node_name[sizeof(_storage_record.node_name) - 1U] = '\0';
    _storage_write_flash();
    printf("[storage] Persisted device name: %s\n", name);
    return true;
}

/**
 * @brief Claim the one-time BLE bond reset for the current firmware.
 *
 * @param void No parameters.
 * @return bool true if reset was successfully claimed, false otherwise.
 */
bool storage_claim_ble_bond_reset(void) {
    if (!_storage_ready || _storage_record.ble_reset_claimed != 0U) {
        return false;
    }
    _storage_record.ble_reset_claimed = 1U;
    _storage_write_flash();
    return true;
}

/**
 * @brief Return the persistent local identity.
 *
 * @param void No parameters.
 * @return const MeshIdentity* Pointer to persistent MeshIdentity structure.
 */
const MeshIdentity *storage_identity(void) {
    return &_identity;
}
