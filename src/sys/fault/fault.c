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

void fault_init(void);

uint32_t fault_read(volatile uint32_t *reg);

void fault_clear(volatile uint32_t *reg, uint8_t flag);

void fault_log(volatile uint32_t *reg, uint8_t flag);

__attribute__((noreturn)) void fault_sw_restart(void) {
    __disable_irq();

    SCB->AIRCR |= SCB_AIRCR_SYSRESETREQ_Msk; // Send software reset request.

    for (;;) {
    } // Waiting for reset to occur.
}