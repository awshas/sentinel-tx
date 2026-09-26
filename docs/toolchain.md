# Toolchain

What builds this firmware, where it came from, and how to prove you have the
same thing. Last updated 2026-09-26.

## Pinned: the cross compiler

The compiler decides the bytes in the binary, so it is pinned exactly and
verified by checksum. CI (`.github/workflows/build.yml`) downloads the same
file and checks the same hash before it builds anything.

| | |
|---|---|
| Package | Arm GNU Toolchain 15.3.Rel1 (Build arm-15.149), `arm-none-eabi` target, x86_64 Linux host |
| Reports as | `arm-none-eabi-gcc (Arm GNU Toolchain 15.3.Rel1 (Build arm-15.149)) 15.3.1 20260627` |
| File | `arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz` |
| Source | Arm's GitLab package registry (15.2.Rel1 and older are on developer.arm.com instead): <br>`https://gitlab.arm.com/api/v4/projects/tooling%2Fgnu-toolchains-for-arm/packages/generic/gnu-toolchain/15.3.rel1/arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz` |
| SHA-256 of the `.tar.xz` | `563bebb2b97d53382b956d6ee1fe61e2cae26699901417234a37df505ef9b5fa` |
| Installed at (dev machine) | `/opt/arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi/`, its `bin/` on `PATH` |

Check a downloaded tarball:

```
echo "563bebb2b97d53382b956d6ee1fe61e2cae26699901417234a37df505ef9b5fa  arm-gnu-toolchain-15.3.rel1-x86_64-arm-none-eabi.tar.xz" | sha256sum --check
```

Check which compiler a build will actually use:

```
arm-none-eabi-gcc --version      # must print 15.3.Rel1 (Build arm-15.149)
grep "set(CMAKE_C_COMPILER " build/CMakeFiles/*/CMakeCCompiler.cmake   # path CMake resolved
```

To use a toolchain that is not on `PATH` (as CI does), configure with
`-DTOOLCHAIN_PREFIX=/path/to/toolchain/bin`.

## Not pinned: host tools from Ubuntu 24.04 (apt)

These drive the build or talk to the board but do not change the firmware
bytes, so any version from the same Ubuntu release is fine.

| Tool | Version seen | Used for |
|---|---|---|
| cmake | 3.28.3 | build configuration (3.20 or newer required) |
| ninja | 1.11.1 | build execution |
| openocd | 0.12.0 | flashing and the GDB server |
| gdb-multiarch | 15.1 | debugging |
| clang-format | 18.1.3 | formatting check (pre-commit hook and CI) |

One exception to "version doesn't matter": clang-format output can change
between major versions, so the hook and CI must use the same major version.
CI installs Ubuntu 24.04's `clang-format` package for that reason.
