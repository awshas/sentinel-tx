# INTERFACE library: compiles nothing, exists only to carry options to targets
# that link it. Keeps strict warnings off third-party code added later
# (TinyUSB in P3, FreeRTOS in P4).
add_library(sentinel_warnings INTERFACE)

# Guarded on COMPILE_LANGUAGE:C so these are not passed when assembling .s.
# Compiler warnings only: linker flags (-Wl,...) belong in target_link_options.
target_compile_options(sentinel_warnings INTERFACE
    $<$<COMPILE_LANGUAGE:C>:
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wdouble-promotion
        -Wundef
    >
)
