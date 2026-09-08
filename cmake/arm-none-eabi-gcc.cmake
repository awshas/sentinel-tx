set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Directory containing the cross toolchain binaries. Leave empty to resolve
# arm-none-eabi-* from PATH; set to a toolchain bin/ directory (trailing slash
# optional) to pin a specific version, e.g. in CI. See docs/toolchain.md.
set(TOOLCHAIN_PREFIX "" CACHE PATH "Path to arm-none-eabi toolchain bin/ directory; empty = use PATH")

if(TOOLCHAIN_PREFIX AND NOT TOOLCHAIN_PREFIX MATCHES "/$")
    set(TOOLCHAIN_PREFIX "${TOOLCHAIN_PREFIX}/")
endif()

set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PREFIX}arm-none-eabi-objcopy")

# Forward the prefix into try_compile sub-projects, which get a fresh cache.
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES TOOLCHAIN_PREFIX)

# No startup code or linker script exists at configure time, so a full link
# would fail. Probe with a static library instead.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Host tools (OpenOCD, python) run on the build machine; libraries and headers
# end up in the ARM binary and must never come from the host.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# -mfpu is deliberately unset: the FPU is unusable until Reset_Handler enables
# CP10/CP11 in SCB->CPACR. Set -mfpu=fpv4-sp-d16 and -mfloat-abi=hard together
# once that is in place.
set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-m4 -mthumb -mfloat-abi=soft")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m4 -mthumb -mfloat-abi=soft")

# nano.specs for newlib-nano. nosys.specs deliberately omitted: syscalls.c
# provides the stubs, so a missing syscall is a link error, not a silent stub.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=cortex-m4 -mthumb -mfloat-abi=soft --specs=nano.specs")