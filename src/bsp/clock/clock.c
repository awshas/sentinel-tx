#include "clock.h"
#include "stm32g4xx.h"
#include "sys/fault/fault.h"
#include "sys/systick/systick.h"

#define HSE_CLK_F_HZ 24000000UL
#define HSI_CLK_F_HZ 16000000UL

PLL_ON
PLL_OFF_FAULT
HSI_FAULT
PLL_OFF
HSE_FAULT
PLL_CONFIG
CLK_SW_FAULT
PLL_FAULT
DEFAULT_STATE
RESTART
DONE

    void
    NMI_Handler(void);

clk_source clock_init(void) {
    // Start the DWT cycle counter
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Start the power clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN_Msk;

    // Check if a software reset was executed on the last clock_init to avoid reset loop.
    systick_init(clock_hclk_hz());
    if (CLK_FLAGS_REG & (1 << CLK_RESET)) {
        fault_clear(CLK_FLAGS_REG, CLK_RESET);
        goto DEFAULT_STATE;
    }

    // Check if the PLL is already off before initiating.
    if (~(RCC->CR & RCC_CR_PLLON_Msk)) {
        goto PLL_OFF;
    }

// PLL is on and must be turned off
PLL_ON:
    // Check if the system clock is running on PLL.
    // Switch to 16MHz HSI if true
    if ((RCC->CFGR & RCC_CFGR_SWS_Msk) == RCC_CFGR_SWS_PLL) {
        RCC->CFGR = ((RCC->CFGR & ~(RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_HSI);
    }

PLL_OFF_FAULT:
HSI_FAULT:
PLL_OFF:
HSE_FAULT:
PLL_CONFIG:
CLK_SW_FAULT:
PLL_FAULT:
DEFAULT_STATE:
RESTART:
DONE:
}

uint32_t clock_hclk_hz(void);

uint32_t clock_pclk1_hz(void);

uint32_t clock_pclk2_hz(void);

uint8_t clock_source(void);