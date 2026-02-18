#!/usr/bin/env python3
"""
Read and decode AVR128DB48 fuses in human-readable format
Usage: python read_fuses.py [programmer] [port]
Examples:
  python read_fuses.py serialupdi COM8
  python read_fuses.py atmelice_updi
  python read_fuses.py atmelice_updi
"""

import subprocess
import sys
import re

# Avrdude paths
AVRDUDE = "C:/Users/uliano/stuff/toolchains/avr-gcc-15.2.0/avr-gcc-15.2.0-x64-windows/bin/avrdude.exe"
AVRDUDE_CONF = "C:/Users/uliano/stuff/toolchains/avr-gcc-15.2.0/avr-gcc-15.2.0-x64-windows/bin/avrdude.conf"
MCU = "avr128db48"

def read_fuse(programmer, port, fuse_num):
    """Read a single fuse value using avrdude"""
    cmd = [AVRDUDE, "-C", AVRDUDE_CONF, "-p", MCU, "-c", programmer]
    if port:
        cmd.extend(["-P", port])
    cmd.extend(["-U", f"fuse{fuse_num}:r:-:h"])

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        # Parse output to extract hex value
        match = re.search(r'0x([0-9a-fA-F]+)', result.stdout)
        if match:
            return int(match.group(1), 16)
        else:
            # Debug: show why parsing failed
            if result.returncode != 0:
                print(f"Avrdude error for fuse{fuse_num}:")
                print(result.stderr)
    except Exception as e:
        print(f"Error reading fuse{fuse_num}: {e}")
    return None

def decode_wdtcfg(value):
    """Decode WDTCFG (fuse0)"""
    period = value & 0x0F
    window = (value >> 4) & 0x0F
    periods = ["OFF", "8ms", "16ms", "32ms", "64ms", "128ms", "256ms", "512ms",
               "1s", "2s", "4s", "8s"]
    print(f"  WDTCFG (fuse0): 0x{value:02X}")
    print(f"    Period: {periods[period] if period < len(periods) else 'Reserved'}")
    print(f"    Window: {periods[window] if window < len(periods) else 'Reserved'}")

def decode_bodcfg(value):
    """Decode BODCFG (fuse1)"""
    sleep = value & 0x03
    active = (value >> 2) & 0x03
    sampfreq = (value >> 4) & 0x01
    lvl = (value >> 5) & 0x07

    modes = ["Disabled", "Enabled", "Sampled", "Enabled (w/ wake)"]
    levels = ["1.9V", "2.6V", "2.9V", "3.3V", "3.7V", "4.0V", "4.2V", "4.5V"]

    print(f"  BODCFG (fuse1): 0x{value:02X}")
    print(f"    Active mode: {modes[active]}")
    print(f"    Sleep mode: {modes[sleep]}")
    print(f"    Sample freq: {'1kHz' if sampfreq else '125Hz'}")
    print(f"    Level: {levels[lvl]}")

def decode_osccfg(value):
    """Decode OSCCFG (fuse2) - AVR128DB48"""
    clksel = value & 0x01
    print(f"  OSCCFG (fuse2): 0x{value:02X}")
    print(f"    CLKSEL: {'External clock' if clksel else 'Internal high-frequency oscillator'}")

def decode_syscfg0(value):
    """Decode SYSCFG0 (fuse5) - AVR128DB48"""
    eesave = (value >> 0) & 0x01
    rstpincfg = (value >> 2) & 0x03
    mvio = (value >> 4) & 0x01
    crcsrc = (value >> 6) & 0x03

    rstmodes = ["GPIO", "Reserved", "RESET", "Reserved"]
    crcsources = ["CRC of FLASH", "CRC of BOOT", "CRC of BOOT+FLASH", "No CRC"]

    print(f"  SYSCFG0 (fuse5): 0x{value:02X}")
    print(f"    EESAVE: {'Yes (preserved)' if eesave else 'No (erased)'}")
    print(f"    RESET pin: {rstmodes[rstpincfg]}")
    print(f"    MVIO: {'Enabled (PORTC dual supply)' if mvio else 'Disabled'}")
    print(f"    CRC source: {crcsources[crcsrc]}")

def decode_syscfg1(value):
    """Decode SYSCFG1 (fuse6)"""
    sut = value & 0x07
    suts = ["0ms", "1ms", "2ms", "4ms", "8ms", "16ms", "32ms", "64ms"]
    print(f"  SYSCFG1 (fuse6): 0x{value:02X}")
    print(f"    Startup time: {suts[sut]}")

def decode_codesize(value):
    """Decode CODESIZE (fuse7)"""
    print(f"  CODESIZE (fuse7): 0x{value:02X}")
    if value == 0:
        print(f"    Application code: Full flash (no separation)")
    else:
        size_kb = value * 256 // 1024
        print(f"    Application code: {size_kb}KB ({value * 256} bytes)")

def decode_bootsize(value):
    """Decode BOOTSIZE (fuse8)"""
    print(f"  BOOTSIZE (fuse8): 0x{value:02X}")
    if value == 0:
        print(f"    Boot section: None")
    else:
        size_kb = value * 256 // 1024
        print(f"    Boot section: {size_kb}KB ({value * 256} bytes)")

def main():
    # Parse arguments
    programmer = sys.argv[1] if len(sys.argv) > 1 else "serialupdi"
    port = sys.argv[2] if len(sys.argv) > 2 and "updi" not in sys.argv[2] else None

    print(f"Reading fuses from AVR128DB48 using {programmer}" + (f" on {port}" if port else ""))
    print("=" * 60)

    # Read all fuses
    fuses = {}
    for fuse_num in [0, 1, 2, 5, 6, 7, 8]:
        fuses[fuse_num] = read_fuse(programmer, port, fuse_num)
        if fuses[fuse_num] is None:
            print(f"Failed to read fuse{fuse_num}")
            return 1

    print()
    print("Fuse Configuration:")
    print("=" * 60)

    # Decode each fuse
    decode_wdtcfg(fuses[0])
    print()
    decode_bodcfg(fuses[1])
    print()
    decode_osccfg(fuses[2])
    print()
    decode_syscfg0(fuses[5])
    print()
    decode_syscfg1(fuses[6])
    print()
    decode_codesize(fuses[7])
    print()
    decode_bootsize(fuses[8])

    return 0

if __name__ == "__main__":
    sys.exit(main())
