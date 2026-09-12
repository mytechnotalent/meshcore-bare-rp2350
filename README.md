![meshcore-bare-rp2350](https://raw.githubusercontent.com/mytechnotalent/meshcore-bare-rp2350/main/meshcore-bare-rp2350.png)

## FREE Reverse Engineering Self-Study Course [HERE](https://github.com/mytechnotalent/reverse-engineering)

<br>

# meshcore-bare-rp2350

`meshcore-bare-rp2350` is bare-metal Raspberry Pi Pico 2 W (RP2350) companion firmware for MeshCore: pure-C SX1262 LoRa radio driver, Ed25519 identity, AES-128 CTR / HMAC-SHA256 authenticated datagram encryption, multi-channel management, and BTstack BLE companion for iPhone and Android.

## Highlights

- RP2350 firmware build using the Raspberry Pi Pico SDK 2.3.1
- Pure-C zero-heap architecture with no dynamic allocations in steady-state operation
- Native Waveshare SX1262 LoRa driver via SPI1 with DIO1 event handling and RF switch control
- Complete MeshCore packet framing (flood routing, path metadata, hop limits)
- Scalable multi-channel support (up to 40 channels) with compact V3 (`'MSC3'`) flash serialization
- BTstack BLE Nordic UART Service (`6E400001-...`) companion radio interface for MeshCore mobile apps
- Enforced Bluetooth pairing with Passkey Entry PIN `123456`, MITM protection, and Display-Only I/O
- Ed25519 node identity generation, signing, and verification
- AES-128 CTR datagram encryption with truncated HMAC-SHA256 authentication
- Persistent RP2350 on-board flash storage for identity keypairs, contacts, channels, and device state
- Comprehensive native Unity test suite (18 test cases) and Python test runner

## Getting Started

Because `meshcore-bare-rp2350` is a bare-metal Pico SDK firmware project, you need the RP2350 toolchain installed on your system.

### 1. Install Toolchain Prerequisites

- Pico SDK 2.3.1+
- ARM GNU toolchain (`arm-none-eabi`)
- CMake and Ninja
- Python 3.x

**Linux:**

```bash
export PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.3.1"
```

**macOS:**

```bash
brew install cmake ninja arm-none-eabi-gcc python
export PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.3.1"
```

**Windows:**

Install PowerShell, Visual Studio Build Tools, CMake, Ninja, Python 3, and the ARM embedded toolchain. The provided build script configures the MSVC environment automatically.

### 2. Build the Firmware

```bash
mkdir -p build && cmake -S . -B build -G Ninja -DPICO_BOARD=pico2_w -DPICO_PLATFORM=rp2350-arm-s && cmake --build build
```

Generated outputs:

- `build/meshcore-bare-rp2350.elf` (primary firmware binary)
- `build/meshcore-bare-rp2350.uf2` (UF2 binary for BOOTSEL/picotool)
- `build/bare-meshcore-rp2350.elf` (backward-compatible copy)
- `build/bare-meshcore-rp2350.uf2` (backward-compatible copy)

### 3. Flash the RP2350

**picotool:**

```bash
picotool load build/meshcore-bare-rp2350.uf2 -fx
```

*(If picotool is not on your PATH, invoke it directly from `$HOME/.pico-sdk/picotool/*/picotool/picotool`).*

**BOOTSEL mode:**

```bash
cp build/meshcore-bare-rp2350.uf2 /Volumes/RP2350/
```

### 4. Connect the Hardware

The firmware targets the Raspberry Pi Pico 2 W paired with a Waveshare SX1262 LoRa module / HAT connected over SPI1:

| Signal | GPIO | Function |
| :--- | :--- | :--- |
| **SCK** | GPIO 10 | SPI1 Clock |
| **MOSI** | GPIO 11 | SPI1 Transmit |
| **MISO** | GPIO 12 | SPI1 Receive |
| **NSS** | GPIO 3 | Chip Select (Active Low) |
| **RESET** | GPIO 15 | Radio Reset |
| **BUSY** | GPIO 2 | Busy Status Indicator |
| **DIO1** | GPIO 20 | Interrupt / Event Signal |
| **RXEN** | GPIO 14 | RF Switch RX Enable |
| **TXEN** | GPIO 13 | RF Switch TX Enable |

## Operating the Node & Companion

Once flashed and powered:

1. The firmware initializes the SX1262 LoRa transceiver at 915.0 MHz (or configured regional frequency) and configures the BTstack BLE companion stack.
2. The board starts BLE advertising under the device name `MeshCore-Bare` (or your persisted customized name).
3. Open the official MeshCore companion app on iOS or Android.
4. Scan for Bluetooth devices and select `MeshCore-Bare`.
5. When prompted, enter the fixed pairing PIN `123456`.
6. Upon authentication, the mobile app connects to the Nordic UART Service (`6E400001-B5A3-F393-E0A9-E50E24DCCA9E`), syncs channel table and contact state, and enables bidirectional text messaging and trace routing over LoRa.

## Architecture & MeshCore Protocol Framing

The firmware implements the complete MeshCore companion framing and over-the-air packet protocol:

- **Transport Routing Envelopes**: Every over-the-air packet begins with a single header byte combining packet type (bits `7..4`) and current hop count (bits `3..0`), followed by a variable-length path array containing node prefixes, and the packet payload.
- **Packet Types**:
  - `0x00`: Trace routing / link path discovery
  - `0x01`: Group / channel datagram
  - `0x02`: Direct node-to-node datagram
  - `0x03`: Node advert broadcast
- **BLE Companion Framing**: Frame delimiters (`0x0A` / `0x0D` / length prefixes) and opcodes for companion commands:
  - `0x01`: App connect / handshake
  - `0x02`: Channel export / sync (`'MSC3'`)
  - `0x03`: Channel import / save
  - `0x04`: Send datagram request
  - `0x05`: Received datagram push event
  - `0x06`: Node status and telemetry push event
  - `0x07`: Set device time

## Cryptographic Security & Channel Architecture

This firmware implements the full MeshCore cryptographic suite:

- **Ed25519 Identity**:
  - 32-byte private seed
  - 32-byte public key (node identity)
  - 64-byte Ed25519 signature verification on broadcast adverts
- **AES-128 CTR Encryption**:
  - 16-byte channel secret key
  - 16-byte counter block (deterministic initialization vector)
  - Symmetric encryption for channel and direct text datagrams
- **HMAC-SHA256 Authentication**:
  - 32-byte key derived from channel / session secret
  - Truncated 4-byte or 8-byte message authentication code (MAC) tag for packet integrity
- **Multi-Channel Management**:
  - Up to 40 channels (`MESHCORE_MAX_CHANNELS 40`)
  - Primary default channel initialized to `"Public"`
  - Compact V3 binary serialization (`'MSC3'` magic header) to minimize flash wear and transfer latency

## Security Notes

The firmware enforces strict cryptographic boundaries:

- BLE GATT characteristics require authenticated and paired connections; unbonded peers cannot inspect or write node configuration.
- Channel keys and identity secrets are stored in dedicated flash sectors with CRC validation and are never transmitted in plaintext over the air.
- Scope boundary: claims here are limited to intended firmware execution on genuine RP2350 silicon; hardware-level side-channel analysis and physical fault injection are out of scope.

### Hardening Checklist

Use this checklist before production deployment:

- Configure flash boot protection / OTP bits on RP2350 to disable SWD debugging in field units.
- Enable hardware watchdog timer to ensure autonomous recovery in case of transceiver brownout.
- Verify RF switch GPIO state transitions to prevent TX power reflection.
- Rotate companion pairing passkey or implement out-of-band dynamic passkeys for mission-critical deployments.

## Running Unit Tests

Run the native Unity unit test suite using the test runner script:

```bash
python3 scripts/run_tests.py
```

Or configure and run via CMake and CTest:

```bash
cmake -S test -B build/test -G Ninja && cmake --build build/test && ctest --test-dir build/test --output-on-failure
```

Or run the Python test adapter for automated discovery:

```bash
python3 -m unittest discover -v -s test -p "test_*.py"
```

To audit code standard compliance:

```bash
python3 scripts/audit_blank_lines.py
python3 scripts/audit_python_standard.py
```

## Project Layout

- `src/main.c`: firmware entry point and cooperative main loop
- `src/runtime.c`: system initialization, timers, and cooperative event scheduler
- `src/sx1262.c`: native SPI1 SX1262 LoRa driver and RF switch control
- `src/mesh_packet.c`: MeshCore packet framing, hop validation, and serialization
- `src/mesh_transport.c`: flood routing, link path tracking, and packet dispatch
- `src/identity.c`: portable Ed25519 identity generation, signing, and verification
- `src/mesh_crypto.c`: AES-128 CTR encryption and HMAC-SHA256 authentication shims
- `src/node_state.c`: in-memory node configuration, channels, and contact table
- `src/storage.c`: non-volatile RP2350 flash storage driver for configuration blobs
- `src/ble_companion.c`: BTstack Nordic UART Service BLE driver with PIN pairing
- `src/companion.c`: companion protocol packet encoding, decoding, and push frames
- `src/datagram.c`: direct and group datagram request processing
- `src/advert.c`: node advert frame construction and verification
- `src/meshcore.gatt`: Bluetooth GATT database specification
- `include/config.h`: hardware pinout, RF configuration, and system limits
- `include/sx1262.h`: SX1262 register definitions and public driver API
- `include/mesh_packet.h`: over-the-air packet definitions and route structures
- `include/mesh_transport.h`: mesh routing and transport API declarations
- `include/identity.h`: Ed25519 cryptographic identity declarations
- `include/mesh_crypto.h`: symmetric cipher and HMAC helper declarations
- `include/node_state.h`: node contact and channel state management API
- `include/storage.h`: persistent flash storage API declarations
- `include/ble_companion.h`: BLE companion interface declarations
- `include/companion.h`: companion framing and opcode definitions
- `include/datagram.h`: datagram construction and parsing declarations
- `include/advert.h`: node advert API declarations
- `include/runtime.h`: runtime event loop and timer definitions
- `include/mbedtls_config.h`: minimal mbedTLS configuration header
- `crypto/ed25519/`: portable Ed25519 cryptographic implementation
- `test/test_channels_and_security.c`: comprehensive Unity unit test suite
- `test/test_meshcore.py`: VS Code test explorer Python test adapter
- `test/unity_config.h`: Unity test configuration header
- `test/unity/`: vendored Unity test framework
- `test/mock/`: hardware mock headers for host testing
- `test/CMakeLists.txt`: CMake build configuration for native unit tests
- `scripts/run_tests.py`: native test runner script
- `scripts/audit_blank_lines.py`: C blank-line compliance scanner
- `scripts/audit_python_standard.py`: Python style and docstring auditor
- `.clang-format`: LLVM C/C++ code formatting rules
- `.clangd`: Clangd LSP configuration for RP2350 ARM Cortex-M33
- `.vscode/`: VS Code workspace settings, tasks, and C/C++ properties
- `meshcore-bare-rp2350.png`: project banner illustration

<br>

## License

MIT — see [LICENSE](LICENSE).
