#include "systick.h"
#include "stm32g4xx.h"
#include "sys/fault/fault.h"
#include <stdint.h>

/* Private Varialbles */
static volatile uint32_t current_ms = 0;

/* Public Functions */
void SysTick_Handler(void) { current_ms++; }

void systick_init(uint32_t core_clk_hz) {
    if (core_clk_hz < SYSTICK_MIN_CORE_CLK_HZ) {
        Error_Handler();
    }
    uint32_t clk_hz_per_ms = core_clk_hz / 1000;
    SysTick->CTRL = 0;
    SysTick->LOAD = (uint32_t)(clk_hz_per_ms - 1UL); // Set RELOAD value
    SysTick->VAL = 0UL;                              // Clear current value
    SysTick->CTRL = (SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                     SysTick_CTRL_ENABLE_Msk); // Set CTRL bits
}

uint32_t systick_millis(void) { return current_ms; }

void systick_delay_ms(uint32_t ms) {
    uint32_t time_stamp = systick_millis();

    // If ms == 1 Then there is a possibility that when delay is called the systick is very close to
    // increasing the current_ms and the wait will be a lot less then 1 ms. That would mean the the
    // delay function will delay between (ms - 1) - ms. That is why we add 1 to garentee the passed
    // amount. ms == UINT32_MAX is checked because UINT32_MAX + 1 = 0 so the delay will be for 0 ms
    // instead of the intended 49.7 days delay.
    uint32_t interval = (ms == UINT32_MAX) ? ms : ms + 1;

    while ((uint32_t)(systick_millis() - time_stamp) < interval)
        ;
}