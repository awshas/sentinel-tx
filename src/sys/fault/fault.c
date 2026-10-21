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