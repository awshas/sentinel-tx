#include "stm32g4xx.h"
#include "sys/systick/systick.h"

#define CPU_CLK_HZ (16000000UL)
_Static_assert(CPU_CLK_HZ >= SYSTICK_MIN_CORE_CLK_HZ,
               "Clock error: Clock frequency must be >= 2000 Hz.");

#define BLINK_INTERVAL_MS (500UL)

// INFO
/* GPIO PA5 is the LED PIN*/

// main loop
int main(void) {
    systick_init(CPU_CLK_HZ);
    // enable GPIOA clock
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN);
    (void)RCC->AHB2ENR; // read-back: the clock enable has reached RCC before GPIOA is touched
    // config GPIO
    GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk;

    GPIOA->MODER |= GPIO_MODER_MODE5_0;
    // Variable declaration

    uint32_t time_stamp = systick_millis();

    while (1) {
        if ((uint32_t)(systick_millis() - time_stamp) >= BLINK_INTERVAL_MS) {
            if (GPIOA->ODR & GPIO_ODR_OD5_Msk) {
                GPIOA->BSRR = GPIO_BSRR_BR5;
            } else {
                GPIOA->BSRR = GPIO_BSRR_BS5;
            }
            time_stamp += BLINK_INTERVAL_MS;
        }
    }
}
