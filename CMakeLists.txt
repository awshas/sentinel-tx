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

include(cmake/warnings.cmake)

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/STM32G474RETX_FLASH.ld")

add_executable(sentinel-tx
    src/main.c
    src/sys/startup_stm32g474.s
)

target_include_directories(sentinel-tx PRIVATE src)

target_link_libraries(sentinel-tx PRIVATE sentinel_warnings)

# One section per function/object, so --gc-sections can drop unused ones.
target_compile_options(sentinel-tx PRIVATE
    -ffunction-sections
    -fdata-sections
)

target_link_options(sentinel-tx PRIVATE
    -T${LINKER_SCRIPT}
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