#include "clock.h"
#include "stm32g4xx.h"
#include "sys/fault/fault.h"
#include "sys/systick/systick.h"

#include <stdbool.h>

#define HSE_CLK_F_HZ 24000000UL
#define HSI_CLK_F_HZ 16000000UL
#define CLOCK_SOURCE (RCC->CFGR & RCC_CFGR_SWS_Msk)

void NMI_Handler(void) {
    RCC->CICR |= RCC_CICR_CSSC; // Clear Clock security system interrupt flag.
    fault_log(NMI_FAULT_FLAGS_REG, NMI_HSE_FAULT);
    systick_init(clock_hclk_hz());
}

bool switch_system_clock(uint32_t clk_value) {
    RCC->CFGR = ((RCC->CFGR & ~(RCC_CFGR_SW_Msk)) | clk_value); // Switch system-clock to clk_value.
    DWT->CYCCNT = 0;
    while (CLOCK_SOURCE != clk_value) // While system-clock is NOT clk_value.
    {
        if (DWT->CYCCNT >=
            16000000UL) // 16 M cycles of waiting this would be atmost 1s if the clock is 16 MHz and
                        // minimum ~94 ms if the clock is 170 MHz.
        {
            return false;
        }
    }
    return true;
}

/*
Configure the clock registers to default state.
- HSE off.
- PLL off.
- AHB,APB1,APB2 all divide by 1.
- Wait states set to 0.
- Power mode set to normal.
*/
void default_state(void) {
    RCC->CR &= ~(RCC_CR_HSEON_Msk); // Turn HSE off.
    RCC->CR &= ~(RCC_CR_PLLON_Msk); // Turn PLL off.
    RCC->CFGR &= ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk |
                   RCC_CFGR_PPRE2_Msk);     // Set AHB, APB1 and APB2 to default values.
    (void)RCC->CFGR;                        // Read back to confirm value.
    FLASH->ACR &= ~(FLASH_ACR_LATENCY_Msk); // Set wait states to 0.
    (void)FLASH->ACR;                       // Read back to confirm values.
    PWR->CR5 |= PWR_CR5_R1MODE;             // Set Power mode to normal.
    (void)PWR->CR5;                         // Read back to confirm value.
}

clk_source clock_init(void) {
    // Timestamp to be used when needed by waits
    uint32_t timestamp = 0;
    // Start the DWT cycle counter
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Start the power clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN_Msk;

    // Check if a software reset was executed on the last clock_init to avoid reset loop.
    if (CLK_FAULT_FLAGS_REG & (1 << CLK_RESET)) {
        fault_clear(CLK_FAULT_FLAGS_REG, CLK_RESET);
        default_state();
        return false;
    }

    // Check if the PLL is already on before initiating.
    if (RCC->CR & RCC_CR_PLLON_Msk) {
        // PLL is on and must be turned off

        if (CLOCK_SOURCE != RCC_CFGR_SWS_HSI) { // Check if the system clock is running on PLL.
                                                // Switch to 16MHz HSI if true.
            if (!(switch_system_clock(RCC_CFGR_SW_HSI))) // Switch system clock to HSI. If timeout
                                                         // go to HSI_FAULT, otherwise continue.
            {
                fault_log(CLK_FAULT_FLAGS_REG, CLK_HSI_FAULT);
                fault_log(CLK_FAULT_FLAGS_REG, CLK_RESET);
                fault_sw_restart();
            }
        }

        systick_init(clock_hclk_hz());
        RCC->CR &= ~(RCC_CR_PLLON);
        timestamp = systick_millis();
        while (RCC->CR & RCC_CR_PLLRDY_Msk) // While PLL locked.
        {
            systick_delay_ms(1);
            if (((uint32_t)(systick_millis() - timestamp)) >= 100UL) {
                fault_log(CLK_FAULT_FLAGS_REG, CLK_PLL_OFF_FAULT);
                fault_log(CLK_FAULT_FLAGS_REG, CLK_RESET);
                fault_sw_restart();
            }
        }
    }

    // HSE configuration
    RCC->CR |= RCC_CR_HSEON; // Turn the HSE on.
    timestamp = systick_millis();
    while ((~(RCC->CR & RCC_CR_HSERDY_Msk)) &&
           (((uint32_t)(systick_millis() - timestamp)) <
            100UL)) // While HSE crystal NOT ready and timeout not reached.
    {
        ;
    }
    if (RCC->CR & RCC_CR_HSERDY) {              // If HSE is on and stable, set source to HSE.
        RCC->CR |= RCC_CR_CSSON;                // Set Clock Security System.
        RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE; // Set PLL source as HSE.
        RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLM_Msk);
        RCC->PLLCFGR |= (RCC_PLLCFGR_PLLM_2 | RCC_PLLCFGR_PLLM_0); // Set PLLM = 6.
    } else { // If timedout, set PLL source to HSI. This will give the 170MHz clock but at a worse
             // accuracy of ±1%.
        fault_log(CLK_FAULT_FLAGS_REG, CLK_HSE_FAULT);
        RCC->CR &= ~(RCC_CR_HSEON); // Disable HSE.
        RCC->PLLCFGR = ((RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLSRC_Msk)) |
                        RCC_PLLCFGR_PLLSRC_HSI); // Set PLL source to HSI;
        RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLM_Msk);
        RCC->PLLCFGR |= (RCC_PLLCFGR_PLLM_1 | RCC_PLLCFGR_PLLM_0); // Set PLLM = 4.
    }

    // PLL configuration
    RCC->PLLCFGR = ((RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLN_Msk)) |
                    (85UL << RCC_PLLCFGR_PLLN_Pos)); // Set PLLN = 85.
    RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLR_Msk)          // Set PLLR = 2.
                     RCC->CR |= RCC_CR_PLLON;        // Turn on PLL.
    timestamp = systick_millis();
    while (~(RCC->CR & RCC_CR_PLLRDY_Msk)) // While PLL NOT locked.
    {
        systick_delay_ms(1);
        if (((uint32_t)(systick_millis() - timestamp)) >= 100UL) {
            goto PLL_FAULT;
        }
    }
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN; // Enable PLLR output

    /*
    Switch from HSI to PLL. Steps in order are:
    1.
    */
    RCC->CFGR = ((RCC->CFGR & ~(RCC_CFGR_HPRE_Msk)) | RCC_CFGR_HPRE_DIV2); // Set AHB prescaler to 2
    RCC->CFGR &= ~(RCC_CFGR_PPRE2_Msk | RCC_CFGR_PPRE1_Msk); // Set APB1 and APB2 to not divided.
    (void)RCC->CFGR;                                         // Read back to confirm change.
    PWR->CR5 &= ~(PWR_CR5_R1MODE_Msk);                       // Set power to boost mode.
    (void)PWR->CR5;                                          // Read back to confirm change.
    FLASH->ACR =
        ((FLASH->ACR & ~(FLASH_ACR_LATENCY_Msk)) | FLASH_ACR_LATENCY_4WS); // Set wait stated to 4.
    (void)FLASH->ACR;                                                      // Read back to confirm.
    if (!(switch_system_clock(RCC_CFGR_SW_PLL))) // Switch system clock to PLL. If timeout go to
                                                 // CLK_SW_FAULT, otherwise continue.
    {
        goto CLK_SW_FAULT;
    }
    DWT->CYCCNT = 0;
    while (
        DWT->CYCCNT <
        85) // Must wait for 1us after PLL switch according to datasheet. at 85MHz 85 cycles is 1us.
    {
        ;
    }
    RCC->CFGR &=
        ~(RCC_CFGR_HPRE_Msk); // Set HPRE to not divide, giving the system clock the full 170MHz.
    (void)RCC->CFGR;          // Read back to confirm change.
    goto DONE;

CLK_SW_FAULT:
    fault_log(CLK_FAULT_FLAGS_REG, CLK_SW_FAULT);
    if (!(switch_system_clock(RCC_CFGR_SW_HSI))) // Switch system clock to HSI. If timeout go to
                                                 // HSI_FAULT, otherwise continue.
    {
        fault_log(CLK_FAULT_FLAGS_REG, CLK_HSI_FAULT);
        fault_log(CLK_FAULT_FLAGS_REG, CLK_RESET);
        fault_sw_restart();
    }
    default_state();
    return false;

PLL_FAULT:
    fault_log(CLK_FAULT_FLAGS_REG, CLK_PLL_FAULT);
    default_state();

DONE:
    systick_init(clock_hclk_hz());
    return true;
}

/* TODO:
- Refactor for repetitive code.
- etc....
*/

uint32_t clock_hclk_hz(void);

uint32_t clock_pclk1_hz(void);

uint32_t clock_pclk2_hz(void);

uint8_t clock_source(void);