# INTERFACE library: compiles nothing, exists only to carry options to targets
# that link it. Keeps strict warnings off third-party code added later
# (TinyUSB in P3, FreeRTOS in P4).
add_library(sentinel_warnings INTERFACE)

# Guarded on COMPILE_LANGUAGE:C so these are not passed when assembling .s.

# With -Wl and --no-warn-rwx-segments because Newer binutils warns when one
# LOAD segment is readable, writable and executable at once. On a Cortex-M
# there's no MMU enforcing page permissions, so the warning describes a
# condition that can't be exploited the way it could on Linux.

target_compile_options(sentinel_warnings INTERFACE
    $<$<COMPILE_LANGUAGE:C>:
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wdouble-promotion
        -Wundef
        -Wl
        --no-warn-rwx-segments  
    >
)