cmake_minimum_required(VERSION 3.20)

# Ninja is single-config: with no build type, CMake adds neither -O nor -g.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

project(sentinel-tx
    LANGUAGES C ASM
    VERSION 0.1.0
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Writes build/compile_commands.json: the exact flags, include paths (CMSIS)
# and defines (STM32G474xx) of every file, so the editor's IntelliSense/clangd
# resolves register names the same way the compiler does.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/warnings.cmake)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/STM32G474RETX_FLASH.ld")

add_executable(sentinel-tx
    src/main.c
    src/sys/startup_stm32g474.s
)

target_include_directories(sentinel-tx PRIVATE src)

# SYSTEM suppresses warnings from third-party headers, so -Werror applies to
# our code without CMSIS breaking the build.
target_include_directories(sentinel-tx SYSTEM PRIVATE
    third_party/CMSIS_5/CMSIS/Core/Include
    third_party/cmsis_device_g4/Include
)

# Selects the device header behind the stm32g4xx.h umbrella.
target_compile_definitions(sentinel-tx PRIVATE STM32G474xx)

target_link_libraries(sentinel-tx PRIVATE sentinel_warnings)

# One section per function/object, so --gc-sections can drop unused ones.
target_compile_options(sentinel-tx PRIVATE
    -ffunction-sections
    -fdata-sections
)

target_link_options(sentinel-tx PRIVATE
    -T${LINKER_SCRIPT}
    # No crt0/crti/crtbegin from the toolchain: startup_stm32g474.s is the only
    # boot code. It also supplies the empty _init/_fini that newlib's
    # __libc_init_array calls.
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=$<TARGET_FILE_DIR:sentinel-tx>/sentinel-tx.map
    -Wl,--print-memory-usage
)

# LINK_DEPENDS: CMake does not treat the linker script as a build input.
# Without it, editing the script does not trigger a relink.
set_target_properties(sentinel-tx PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS "${LINKER_SCRIPT}"
)

add_custom_command(TARGET sentinel-tx POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:sentinel-tx> $<TARGET_FILE_DIR:sentinel-tx>/sentinel-tx.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:sentinel-tx> $<TARGET_FILE_DIR:sentinel-tx>/sentinel-tx.hex
    COMMENT "Generating .bin and .hex"
)