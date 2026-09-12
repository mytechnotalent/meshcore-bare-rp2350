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
// File:    mbedtls_config.h
// Desc:    Configures minimal mbedTLS cryptographic primitives for MeshCore.
// Created: 2026

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#include <limits.h>

/**
 * @brief Enable AES block cipher.
 */
#define MBEDTLS_AES_C

/**
 * @brief Enable AES Counter (CTR) mode.
 */
#define MBEDTLS_CIPHER_MODE_CTR

/**
 * @brief Enable generic message digest (MD) abstraction.
 */
#define MBEDTLS_MD_C

/**
 * @brief Enable SHA-256 hash algorithm.
 */
#define MBEDTLS_SHA256_C

/**
 * @brief Enable generic cipher layer.
 */
#define MBEDTLS_CIPHER_C

/**
 * @brief Enable platform abstraction layer.
 */
#define MBEDTLS_PLATFORM_C

#endif // MBEDTLS_CONFIG_H
