#include "stm32g4xx.h"

#define CPU_CLK_HZ (16000000UL)
#define CYCLES_PER_ITIRATION (11)
#define BLINK_HALF_PERIOD_LOOPS (CPU_CLK_HZ / CYCLES_PER_ITIRATION / 2U)

// INFO
/* GPIO PA5 is the LED PIN*/

// main loop
int main(void) {
    // enable GPIOA clock
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);
    (void)RCC->AHB2ENR; // read-back: the clock enable has reached RCC before GPIOA is touched
    // config GPIO
    GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk;
    GPIOA->MODER |= GPIO_MODER_MODE5_0;
    while (1) {
        GPIOA->BSRR = (GPIO_BSRR_BS5);
        for (volatile uint32_t i = 0; i < (BLINK_HALF_PERIOD_LOOPS); i++)
            ;
        GPIOA->BSRR = (GPIO_BSRR_BR5);
        for (volatile uint32_t i = 0; i < (BLINK_HALF_PERIOD_LOOPS); i++)
            ;
    }
}