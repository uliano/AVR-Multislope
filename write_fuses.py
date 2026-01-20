#!/usr/bin/env python3
"""
Write fuses to AVR128DA48 using avrdude
Usage: python write_fuses.py [programmer] [port]
Examples:
  python write_fuses.py serialupdi COM8
  python write_fuses.py atmelice_updi
"""

import subprocess
import sys

# Avrdude paths
AVRDUDE = "C:/Users/uliano/stuff/toolchains/avr-gcc-15.2.0/avr-gcc-15.2.0-x64-windows/bin/avrdude.exe"
AVRDUDE_CONF = "C:/Users/uliano/stuff/toolchains/avr-gcc-15.2.0/avr-gcc-15.2.0-x64-windows/bin/avrdude.conf"
MCU = "avr128da48"

# Fuse values (from platformio.ini)
FUSES = {
    "fuse0": 0x00,  # WDTCFG - Watchdog disabled
    "fuse1": 0x00,  # BODCFG - BOD disabled
    "fuse2": 0x02,  # OSCCFG - 24MHz internal oscillator
    "fuse5": 0xC8,  # SYSCFG0 - EESAVE=no, RESET pin, no CRC
    "fuse6": 0x07,  # SYSCFG1 - 64ms startup time
    "fuse7": 0x00,  # CODESIZE - Full flash for application
    "fuse8": 0x00,  # BOOTSIZE - No bootloader section
}

def write_fuses(programmer, port):
    """Write all fuses using avrdude"""
    cmd = [AVRDUDE, "-C", AVRDUDE_CONF, "-p", MCU, "-c", programmer]
    if port:
        cmd.extend(["-P", port])

    # Add all fuse writes
    for fuse_name, fuse_value in FUSES.items():
        cmd.extend(["-U", f"{fuse_name}:w:0x{fuse_value:02X}:m"])

    print(f"Programming fuses to {MCU} using {programmer}" + (f" on {port}" if port else ""))
    print("=" * 60)
    print("Fuse values:")
    for fuse_name, fuse_value in FUSES.items():
        print(f"  {fuse_name}: 0x{fuse_value:02X}")
    print("=" * 60)
    print()
    print("Command:", " ".join(cmd))
    print()

    try:
        result = subprocess.run(cmd, timeout=30)
        if result.returncode == 0:
            print()
            print("=" * 60)
            print("✓ Fuses programmed successfully!")
            print("=" * 60)
            return 0
        else:
            print()
            print("=" * 60)
            print("✗ Failed to program fuses")
            print("=" * 60)
            return 1
    except Exception as e:
        print(f"Error: {e}")
        return 1

def main():
    # Parse arguments
    programmer = sys.argv[1] if len(sys.argv) > 1 else "serialupdi"
    port = sys.argv[2] if len(sys.argv) > 2 and "updi" not in sys.argv[2] else None

    return write_fuses(programmer, port)

if __name__ == "__main__":
    sys.exit(main())
