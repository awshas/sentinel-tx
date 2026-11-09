#ifndef FAULT_H
#define FAULT_H

#include "stm32g4xx.h"
#include <stdint.h>

/* This module is used to keep track of possible faults and errors that the product might encounter.
It saves fault flags and can be read and written using the functions that are declared.

This module MUST be initiated first in main before any other commands.*/

#define BOOT_FAULT_FLAGS_REG (&(TAMP->BKP0R)) // the address of the RCC_CSR reset cause flags.
#define CLK_FAULT_FLAGS_REG (&(TAMP->BKP1R))  // the address of the clock fault flags.
#define NMI_FAULT_FLAGS_REG (&(TAMP->BKP2R))  // the address of the NMI fault flags.

enum clk_flags {
    CLK_SW_HSI_FAULT = 0,   // Clock switch to HSI timed out.
    CLK_HSE_FAULT = 1,      // HSE did not stabilize and timed out.
    CLK_PLL_OFF_FAULT = 2,  // PLL did not turn off and timed out.
    CLK_PLL_FAULT = 3,      // PLL did not stabilize and timed out.
    CLK_SW_FAULT = 4,       // Clock Switch timed out.
    CLK_FLASH_WS_FAULT = 5, // Wait states did not update.
    CLK_RESET = 6,          // Software reset flag.
};

enum nmi_flags {
    NMI_HSE_FAULT = 0,   // CSS got triggered.
    NMI_OTHER_FAULT = 1, // a NMI interrupt but is not related to HSE/CSS.
};

__attribute__((noreturn)) void Error_Handler(void);

void fault_init(void);

// Returns the fault register in uint32_t type.
uint32_t fault_read(volatile uint32_t *reg);

// Clears the given flag in the given fault register.
void fault_clear(volatile uint32_t *reg, uint32_t flag);

// Sets the given flag in the given fault register.
void fault_log(volatile uint32_t *reg, uint32_t flag);

// Request software restart and waits until executed
__attribute__((noreturn)) void fault_sw_restart(void);

#endif