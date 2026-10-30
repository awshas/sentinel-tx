#ifndef FAULT_H
#define FAULT_H

#include "stm32g4xx.h"
#include <stdint.h>

// This module should handle fault flags and TAMP register init etc (this is a reminder that will
// get deleted)

/* This module is used to keep track of possible faults and errors that the product might encounter.
It saves fault flags and can be read and written using the functions that are declared.

This module MUST be initiated first in main before any other commands.*/

#define CLK_FAULT_FLAGS_REG (TAMP->BKP0R)
#define NMI_FAULT_FLAGS_REG (TAMP->BKP1R)

enum clk_flags {
    CLK_HSI_FAULT,
    CLK_HSE_FAULT,
    CLK_PLL_OFF_FAULT,
    CLK_PLL_FAULT,
    CLK_SW_FAULT,
    CLK_RESET // Software reset flag
};

enum nmi_flags { NMI_HSE_FAULT };

__attribute__((noreturn)) void Error_Handler(void);

void fault_init(void);

uint32_t fault_read(volatile uint32_t *reg);

void fault_clear(volatile uint32_t *reg, uint8_t flag);

void fault_log(volatile uint32_t *reg, uint8_t flag);

__attribute__((noreturn)) void fault_sw_restart(void);

#endif