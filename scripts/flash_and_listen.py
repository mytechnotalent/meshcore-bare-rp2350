#!/usr/bin/env python3
"""Flash firmware to device and monitor serial output."""
import subprocess
import threading
import time
import serial

SERIAL_PORT = "/dev/cu.usbmodem101"
BAUD_RATE = 115200


def read_serial(timeout_seconds: float = 8.0) -> list[str]:
    """
    Connect to serial port and read lines for specified duration.

    Parameters
    ----------
    timeout_seconds : float
        Total duration to listen in seconds.

    Returns
    -------
    list[str]
        Collected output lines.
    """
    lines = []
    end_time = time.time() + timeout_seconds
    ser = None
    while time.time() < end_time and ser is None:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        except Exception:
            time.sleep(0.05)
    if ser is None:
        print("[HOST] Could not open serial port within timeout")
        return lines
    with ser:
        print("[HOST] Serial port connected, reading...")
        while time.time() < end_time:
            line = ser.readline()
            if line:
                text = line.decode("utf-8", errors="replace").strip()
                print(f"[DEVICE] {text}")
                lines.append(text)
    return lines


def main() -> None:
    """
    Flash firmware via picotool and monitor output.

    Parameters
    ----------
    None

    Returns
    -------
    None
    """
    print("Flashing firmware with picotool...")
    res = subprocess.run(
        ["picotool", "load", "-f", "-x", "build/meshcore-bare-rp2350.elf"],
        capture_output=True,
        text=True,
    )
    print("Picotool output:", res.stdout, res.stderr)
    read_serial(timeout_seconds=8.0)


if __name__ == "__main__":
    main()


