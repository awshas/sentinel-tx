#include "fault.h"
#include "stm32g4xx.h"

__attribute__((noreturn)) void Error_Handler(void) {
    __disable_irq();

    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        __BKPT(0); // halts here only with a debugger attached; without one, BKPT faults
    }

    for (;;) {
    }
}

void fault_init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN; // Enable PWR clock to access power registers.
    (void)RCC->APB1ENR1;                 // Read back to confirm.

    RCC->APB1ENR1 |= RCC_APB1ENR1_RTCAPBEN; // Enable RTC APB clock to access TAMP registers.
    (void)RCC->APB1ENR1;                    // Read back to confirm.

    PWR->CR1 |= PWR_CR1_DBP; // Enable access to RTC and Backup registers.
    (void)PWR->CR1;          // Read back to confirm.

    *BOOT_FAULT_FLAGS_REG = RCC->CSR; // Save Control/Status flags from last boot.
    RCC->CSR |= RCC_CSR_RMVF;         // Reset the Control/Status flags register.
}

uint32_t fault_read(volatile uint32_t *reg) { return *reg; }

void fault_clear(volatile uint32_t *reg, uint32_t flag) { *reg &= (uint32_t) ~(1UL << flag); }

void fault_log(volatile uint32_t *reg, uint32_t flag) { *reg |= (uint32_t)(1UL << flag); }

__attribute__((noreturn)) void fault_sw_restart(void) {
    __disable_irq();

    NVIC_SystemReset();
}