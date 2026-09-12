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
// File:    runtime.h
// Desc:    Declares the MeshCore radio and persistence runtime loop.
// Created: 2026

#ifndef RUNTIME_H
#define RUNTIME_H

/**
 * @brief Run one iteration of the MeshCore radio and persistence runtime loop.
 *
 * Polls the SX1262 LoRa receiver, parses received frames (advert, text, group
 * text, trace, response), dispatches companion events, flushes queued BLE
 * responses, and executes periodic tasks such as dirty-state persistence.
 *
 * @param void No parameters.
 * @return void
 */
void runtime_step(void);

/**
 * @brief Start and run the main MeshCore runtime loop.
 *
 * Executes the continuous radio polling and companion event loop.
 *
 * @param void No parameters.
 * @return void
 */
void runtime_run(void);

#endif // RUNTIME_H
