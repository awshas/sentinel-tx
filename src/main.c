#include "bsp/clock/clock.h"
#include "stm32g4xx.h"
#include "sys/fault/fault.h"
#include "sys/systick/systick.h"

#define BLINK_INTERVAL_MS (500UL)

int main(void) {
    fault_init(); // Initiate fault

    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN); // enable GPIOA clock
    (void)RCC->AHB2ENR;                         // Read back to confirm change

    // Config GPIOA bit 5 as digital output.
    GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk;
    GPIOA->MODER |= GPIO_MODER_MODE5_0;

    clock_init(); // Initiate clock to 170 MHz

    uint32_t time_stamp = systick_millis();

    // main loop
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
