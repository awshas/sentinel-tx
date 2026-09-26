# sentinel-tx

## Description

xxxxxxxxx

## Development Setup
1. git config core.hooksPath .githooks
2. toolchain

## Naming Conventions

When naming .c files or any firmware files no white spaces are allowed. The .githook to check clang-format wont work with files that have white spaces.

## Building

Configure (once per build directory, or after editing the toolchain file):

```
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake -B build -G Ninja
```

Build:
```
cmake --build build
cmake --build build --clean-first     # force full rebuild
```

Fresh start after toolchain edits. CMAKE_TOOLCHAIN_FILE is only read when the cache is created, so an edit to it is invisible otherwise:

```
rm -rf build && cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake -B build -G Ninja
```

Release build, separate directory so both exist at once:

```
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake -DCMAKE_BUILD_TYPE=Release -B build/release -G Ninja
```

Inspect:

```
arm-none-eabi-size build/sentinel-tx.elf
arm-none-eabi-objdump -h build/sentinel-tx.elf
arm-none-eabi-nm -n build/sentinel-tx.elf
arm-none-eabi-objdump -d build/sentinel-tx.elf
```

Flash with OpenOCD:
```
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
        -c "program build/sentinel-tx.elf verify reset exit"
```

Or with ST's CLI:
```
STM32_Programmer_CLI -c port=SWD -w build/sentinel-tx.elf -v -rst
```

Debug session — OpenOCD in one terminal:
```
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg
```

GDB in another:
```
gdb-multiarch build/sentinel-tx.elf
(gdb) target extended-remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) break Reset_Handler
(gdb) continue
```