#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

typedef enum { CLK_SRC_HSI, CLK_SRC_HSI_PLL, CLK_SRC_HSE_PLL } clk_source;

/* Boost MCU CLK frequency to 170MHz.

fault_init() must run first,

It tries HSE with the PLL, then HSI with the PLL, then HSI alone.

It returns which one it got.

It re-initialises SysTick.

It can reset the MCU, after setting the CLK_RESET_LOOP flag in fault.h. */
clk_source clock_init(void);

// Returns the current MCU HCLK speed in Hz
uint32_t clock_hclk_hz(void);

// Returns the current MCU APB1 speed in Hz
uint32_t clock_pclk1_hz(void);

// Returns the current MCU APB2 speed in Hz
uint32_t clock_pclk2_hz(void);

// Returns the current clock source
uint8_t clock_source(void);

#endif