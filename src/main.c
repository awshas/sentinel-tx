#include "stm32g4xx.h"

#define CPU_F (16000000UL)
#define cycles_per_itiration (13)

// INFO
/* GPIO PA5 is the LED PIN*/

// main loop
int main(void) {
    // enable GPIOA clock
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);
    READ_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN); // Delay after an RCC peripheral clock enable.
    // config GPIO
    GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk;
    GPIOA->MODER |= GPIO_MODER_MODE5_0;
    while (1) {
        GPIOA->BSRR = (GPIO_BSRR_BS5);
        for (volatile uint32_t i = 0; i < ((CPU_F / cycles_per_itiration) >> 1); i++)
            ;
        GPIOA->BSRR = (GPIO_BSRR_BR5);
        for (volatile uint32_t i = 0; i < ((CPU_F / cycles_per_itiration) >> 1); i++)
            ;
    }
}