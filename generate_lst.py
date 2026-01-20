#!/usr/bin/env python3
"""
Post-build script for PlatformIO to generate disassembly listing (.lst file)
with source code interleaved using avr-objdump
"""

Import("env")  # type: ignore
import os
import shutil

def generate_listing(source, target, env):  # pylint: disable=unused-argument
    """Generate .lst file with disassembled code and source"""

    # Get the firmware.elf path
    firmware_elf = str(target[0])

    # Generate output paths
    build_dir = os.path.dirname(firmware_elf)
    firmware_lst = os.path.join(build_dir, "firmware.lst")
    firmware_map = os.path.join(build_dir, "firmware.map")

    # Get objdump from the toolchain
    objdump = env.get("OBJDUMP", "avr-objdump")

    # Generate disassembly with source code (-S), all headers (-x), and line numbers (-l)
    print("Generating disassembly listing: %s" % firmware_lst)
    env.Execute(
        "%s -h -S -z %s > %s" % (objdump, firmware_elf, firmware_lst)
    )

    # Copy firmware.map to build directory if it exists in project root
    map_in_root = "firmware.map"
    if os.path.exists(map_in_root):
        print("Moving linker map file: %s -> %s" % (map_in_root, firmware_map))
        shutil.move(map_in_root, firmware_map)

    print("Build artifacts generated:")
    print("  - ELF:    %s" % firmware_elf)
    print("  - MAP:    %s" % firmware_map)
    print("  - LST:    %s" % firmware_lst)

# Add post-build action
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", generate_listing)  # type: ignore
