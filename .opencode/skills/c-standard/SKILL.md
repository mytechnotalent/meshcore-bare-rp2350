---
name: c-standard
description: The C coding standard for the meshcore-bare-rp2350 pure-C firmware. Use when writing, editing, or reviewing any C code (.c/.h) in this repo, including Doxygen documentation compliance, the no-blank-line-inside-function rule, files covered, and the audit method. Trigger on keywords like "C standard", "style", "documentation", "Doxygen", "doc compliance", "blank line", "code audit", or when starting any new function, struct, macro, or file.
---

# C Standard (meshcore-bare-rp2350)

This is the coding standard for the pure-C firmware and test suite in this repo.
Every function, global, macro, `typedef`, struct, and member must comply.
Vendored libraries and auto-generated files are **excluded** (see Scope).

## Scope: what must comply

Comply (all C code you own):

- Every `*.c` and `*.h` in this repo that you write or edit, including `src/`,
  `include/`, and `test/` (everything except the exclusions below).

Excluded (do not touch):

- `crypto/ed25519/` (vendored Ed25519 cryptographic library)
- `test/unity/` (vendored Unity test framework)
- Vendored or fetched third-party libraries (e.g. Pico SDK, BTstack)

## Rule 1: No blank lines inside a function or method body

There must be **zero** blank lines within any function body.

- A function body is the `{ ... }` that follows a signature — i.e. a `{` whose
  preceding non-whitespace character is `)`. In C this covers function
  definitions AND the control-flow blocks inside them (`if/for/while/switch`):
  all of those braces are inside a function body, so blank lines are forbidden
  at any depth within the body.
- Blank lines are allowed **only** to separate functions or types from each
  other (outside bodies). Struct/union/enum member declarations are **not**
  function bodies — blank lines may separate them.
- A "blank line" is a line that is whitespace-only (including a line of indent
  spaces). Lines that contain code, a `//` comment, or a `/** ... */` block are
  NOT blank and are untouched.

Examples:

```c
// WRONG: blank lines (line 2 and 4) inside the body
static void clear_bytes(uint8_t *buf, size_t len) {
    size_t i;
                      // <- blank, remove
    for (i = 0u; i < len; ++i) {
        buf[i] = 0u;
    }
}
```

```c
// CORRECT: no blank lines inside the body
static void clear_bytes(uint8_t *buf, size_t len) {
    size_t i;
    for (i = 0u; i < len; ++i) {
        buf[i] = 0u;
    }
}
```

### Comments inside bodies

- Keep comment lines in place; only remove the genuinely blank lines.
- A multi-line `/* ... */` comment can span blank-looking comment lines:
  those are comment content, not blank lines — leave them. Only whitespace-only
  lines count.

## Rule 2: Doxygen documentation compliance (100%)

Every entity in scope must carry Doxygen documentation.

### File header

Every file starts with this exact structure (standard MIT header followed
by the Doxygen block):

```c
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
// File:    filename.c
// Desc:    Description of module.
// Created: 2026
```

### Functions

Every function (including file-local `static` helpers and test cases) gets a
doc block immediately above it with `@brief` and, when applicable,
`@param`/`@return`:

```c
/**
 * @brief Initialize the MeshCore node state module.
 *
 * @param void No parameters.
 * @return void
 */
void node_state_init(void) {
    memset(_contacts, 0, sizeof(_contacts));
    memset(_channels, 0, sizeof(_channels));
}
```

Use `@param void No parameters.` / `@return void` when there are none
meaningful. If a function takes parameters, name every one:

```c
/**
 * @brief Set the advertised node name.
 *
 * @param name Pointer to raw name bytes.
 * @param length Length of name in bytes.
 * @return bool true if name set successfully, false otherwise.
 */
bool node_state_set_name(const uint8_t *name, size_t length);
```

### Globals, `#define`s, `typedef`s, struct members, and constants

Each gets a `@brief` block:

```c
/**
 * @brief SX1262 SPI clock pin.
 */
#define MESHCORE_PIN_SCK 10u

/**
 * @brief Maximum MeshCore group channels.
 */
#define MESHCORE_MAX_CHANNELS 40u
```

For struct and enum types, each member declaration is preceded by a `@brief`
block:

```c
/**
 * @brief MeshCore payload type enumeration.
 */
typedef enum {
    /**
     * @brief Request payload.
     */
    MESH_PAYLOAD_REQUEST = 0,
    /**
     * @brief Response payload.
     */
    MESH_PAYLOAD_RESPONSE = 1,
} MeshPayloadType;
```

### Locals

Simple loop iteration locals need no annotation. Named/explicit locals carry a
`@brief Declaration of X.` block immediately above (no blank line between the
comment and the declaration, and no blank line between consecutive blocks):

```c
static void test_sample(void) {
    /**
     * @brief Declaration of result.
     */
    bool result;
    /**
     * @brief Declaration of phrase.
     */
    const uint8_t phrase[] = "test phrase";
    result = node_state_set_name(phrase, sizeof(phrase) - 1);
    TEST_ASSERT_TRUE(result);
}
```

> Note: the `@brief Declaration of <name>.` blocks are a repo convention and
> MUST be preserved when present. Blank lines inside function bodies are the
> only thing removed — never delete a Doxygen comment block.

## Rule 3: Naming and structure conventions

- Files: `snake_case` (`sx1262.c`, `mesh_packet.c`, `test_channels_and_security.c`).
- Constants/`#define`s: `UPPER_SNAKE` (`MESHCORE_PIN_SCK`, `MESHCORE_DEFAULT_FREQUENCY`).
- Types (`typedef` names): `PascalCase` (`MeshPacket`, `NodeContact`, `MeshIdentity`) or `snake_case_t`.
- Functions: `snake_case`, prefixed with the module name (`node_state_init`,
  `sx1262_transmit`, `companion_receive`).
- Struct/enum members: `snake_case`.
- File-local (static) symbols: `_snake_case` private helpers or variables.
- `#include` guards or `#pragma once` at the top of headers.

## Rule 4: Pitfalls to avoid

- Never include secrets or credentials in plaintext readably in the binary.
- Never use leading-underscore+capital identifiers (`_Foo`) — reserved.
- Don't touch vendored `crypto/ed25519/` or `test/unity/`.
- Don't add `//` editor comments; keep the Doxygen style consistent (`/** */`).
- Use `<stdint.h>` fixed-width types; never rely on plain `int` width for wire
  data or cryptographic state.
- No implicit narrowing conversions that truncate values.
- Keep stack usage bounded for RP2350 microcontroller execution.

## Auditing: how to verify compliance

Run the audit scanner to find blank lines inside function bodies:

```bash
python3 scripts/audit_blank_lines.py
```

Run Python tool auditing:

```bash
python3 scripts/audit_python_standard.py
```

Run the unit test suite:

```bash
python3 scripts/run_tests.py
```

Rebuild the firmware using Pico SDK:

```bash
mkdir -p build && cmake -S . -B build -G Ninja -DPICO_BOARD=pico2_w -DPICO_PLATFORM=rp2350-arm-s && cmake --build build
```
