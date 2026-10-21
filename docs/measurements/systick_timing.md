# SysTick Timing

How long the LD2 blink period really is, first with a busy-wait delay and then
with the SysTick time base, measured in CPU cycles with the DWT cycle counter.
Measured 2026-10-05 to 2026-10-21 on `feat/blinky` (squash-merged as 158d7c6)
and `feat/systick`.

## Setup

| | |
|---|---|
| Board | NUCLEO-G474RE, LD2 on PA5 |
| Clock | HSI16 (16 MHz) as SYSCLK, no PLL, 0 flash wait states (reset defaults) |
| Compiler | Arm GNU Toolchain 15.3.Rel1 (see `docs/toolchain.md`) |
| Debug build | `-g`, no `-O`, so `-O0` |
| Release build | `-O3 -DNDEBUG` |
| Debugger | OpenOCD 0.12 and `gdb-multiarch` 15.1 over the on-board STLINK-V3E |

At 16 MHz, 1 ms = 16,000 cycles and 500 ms = 8,000,000 cycles. Cycle counts
depend on the compiler version and flags; re-measure after changing either.

## Results

### Busy-wait delay (blinky)

`for (volatile uint32_t i = 0; i < N; i++);`

| Build | Instructions per iteration | Cycles per iteration |
|---|---|---|
| Debug | 7 (reloads the loop limit from flash every pass) | 13 |
| Release | 6 (keeps the limit in a register) | 11 |

- Debug: 14,950,014 cycles for 1,150,000 iterations = 13 per iteration plus
  14 cycles of overhead.
- Release: 13,538,463 cycles per blink period = 2 × 615,384 × 11 + 15.

The same `N` gives a different blink rate in each build. A delay calibrated in
loop iterations is only valid for the binary it was calibrated on.

Release builds, one change at a time:

| Clock-enable read-back | Loop counter `volatile` | LD2 |
|---|---|---|
| no | no | stuck |
| yes | no | stuck off |
| no | yes | blinks, `GPIOA_MODER` = 0xABFFF7FF |
| yes | yes | blinks |

### SysTick time base (1 ms tick, non-blocking main loop)

| Measurement | Debug | Release |
|---|---|---|
| First poll after `systick_init()` to first toggle | 8,000,014 cycles | 7,999,994 cycles |
| One main-loop pass | not measured | 11 cycles (about 1.45 million passes/s) |

Expected: 8,000,000 cycles in both builds. The period no longer depends on
the build. The ±20 cycles come from where the 500th tick lands inside a loop
pass, and from the window starting at the first poll instead of at a toggle.
The blocking version made one main-loop pass per second.

Wrap test: with the tick counter starting at 0xFFFFF000, the counter wraps
about 4.1 s after boot. LD2 kept blinking through the wrap.

## Findings

From `objdump` and GDB on these builds:

1. **Non-volatile busy-wait (Release):** the two `BSRR` stores ended up back to
   back (set, then reset, at 0x08000200 and 0x08000202 in that build). LD2 is
   lit for about one instruction per period, so it looks off.
2. **RCC clock-enable hazard not observed:** with `volatile` but no read-back,
   Release still blinked, `GPIOA_MODER` read 0xABFFF7FF (PA5 output, SWD pins
   unchanged) and SWD kept working. The original stuck LED was the delay
   loops, not RCC timing. The read-back stays, because RM0440 §7.2.17 requires
   a delay after a clock enable.
3. **Non-volatile tick counter:** `systick_delay_ms()` compiled to
   `adds r0, #1; bne; bx lr; b .`. It computes `ms + 1` once, then spins on
   itself forever unless `ms + 1` wraps to 0. The counter is never re-read.
4. **main still worked without `volatile`,** only because `systick_millis()` is
   in another `.c` file, so main must call it (`bl`) on every pass. Link-time
   optimization (`-flto`) would inline it and break this. `volatile` is the
   actual guarantee.
5. **Dead code removed at -O3:** a loop-pass counter that nothing read, and a
   `> 0xFFFFFF` reload check that can never be true (`uint32_t / 1000` is at
   most 4,294,967).
6. **Unused functions are not in the ELF:** with `-ffunction-sections` and
   `--gc-sections`, the unused `systick_delay_ms` appears under "Discarded
   input sections" in the map file. Disassemble the object file instead:
   `build/release/CMakeFiles/sentinel-tx.dir/src/sys/systick/systick.c.obj`.
7. **Off-by-one in LOAD (calculated):** `LOAD = 16000` gives 16,001 cycles per
   tick, 62.5 ppm slow, about 5.4 s per day. `LOAD = cycles_per_tick - 1` is
   correct.

## Limits

DWT and SysTick both count CPU clock cycles, so these numbers prove that a tick
is exactly 16,000 cycles. They say nothing about how long a cycle is: HSI16 is
±1 % over 0–85 °C (DS12288). Real-time accuracy needs an external reference
(scope or logic analyser on PA5), or the 24 MHz HSE crystal (20 ppm) once
`clock.c` uses it.

## How to reproduce

Don't use VS Code F5 for Release measurements: it builds and flashes the Debug
ELF. Use two terminals:

```
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg
```

```
gdb-multiarch build/release/sentinel-tx.elf
(gdb) target extended-remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) disassemble main
(gdb) tbreak *0x08000218
(gdb) continue
(gdb) set *(unsigned int *)0xE000EDFC |= (1 << 24)
(gdb) set *(unsigned int *)0xE0001000 |= 1
(gdb) set *(unsigned int *)0xE0001004 = 0
(gdb) tbreak *0x08000218
(gdb) continue
(gdb) p/u *(unsigned int *)0xE0001004
```

- The three `set` lines enable DEMCR.TRCENA, enable DWT_CTRL.CYCCNTENA and zero
  DWT_CYCCNT. GDB has no names for these registers, since the Release ELF has no
  debug info, so they are written by address.
- 0x08000218 was the first instruction of the toggle in that build. Addresses
  change with every build, so find them with `disassemble main` first.
- Break on an instruction that runs exactly once per period. Two hits on the
  toggle give one full period. For one main-loop pass, break twice on the poll
  instruction instead (0x0800020c in that build).
- SysTick and CYCCNT both stop while the core is halted, so the time spent at a
  breakpoint doesn't count. That's why one loop pass reads 11 cycles, not
  thousands.
