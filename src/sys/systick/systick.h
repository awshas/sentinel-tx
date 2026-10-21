#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

#define SYSTICK_MIN_CORE_CLK_HZ (2000UL) // 2000 Hz

// An init for the Systick that takes the clock frequncy in Hz. It can be reinitilized.
void systick_init(uint32_t core_clk_hz);

// Returns the total milli seconds passed since init ran. It will wrap after ~49.7 days.
uint32_t systick_millis(void);

// A blocking delay function that will block for the amount of milli seconds passed as the
// parameter.
void systick_delay_ms(uint32_t ms);

#endif