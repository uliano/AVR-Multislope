# PlatformIO extra script to add toolchain include paths for bare metal builds
# This fixes IntelliSense by providing the AVR system headers

Import("env")
import os

# Get the toolchain package directory
toolchain_dir = env.PioPlatform().get_package_dir("toolchain-atmelavr")

if toolchain_dir:
    # Add AVR libc includes
    avr_include = os.path.join(toolchain_dir, "avr", "include")

    # Add GCC includes (find the version directory)
    gcc_lib_dir = os.path.join(toolchain_dir, "lib", "gcc", "avr")
    gcc_include = None
    if os.path.isdir(gcc_lib_dir):
        # Find the GCC version directory (e.g., "15.2.0")
        versions = [d for d in os.listdir(gcc_lib_dir) if os.path.isdir(os.path.join(gcc_lib_dir, d))]
        if versions:
            gcc_include = os.path.join(gcc_lib_dir, versions[0], "include")

    # Append to CPPPATH
    paths_to_add = []
    if os.path.isdir(avr_include):
        paths_to_add.append(avr_include)
    if gcc_include and os.path.isdir(gcc_include):
        paths_to_add.append(gcc_include)

    if paths_to_add:
        env.Append(CPPPATH=paths_to_add)
        print("Added toolchain include paths:")
        for p in paths_to_add:
            print("  ", p)

# Add the MCU-specific define that GCC generates internally with -mmcu
# This is needed for IntelliSense to parse AVR headers correctly
mcu = env.BoardConfig().get("build.mcu", "")
if mcu:
    # Convert mcu name to the define format: avr128da48 -> __AVR_AVR128DA48__
    mcu_define = "__AVR_" + mcu.upper() + "__"
    env.Append(CPPDEFINES=[mcu_define])
    print("Added MCU define:", mcu_define)
