#include "clock.h"
#include "stm32g4xx.h"
#include "sys/fault/fault.h"
#include "sys/systick/systick.h"

#include <stdbool.h>

#define HSE_CLK_HZ 24000000UL
#define HSI_CLK_HZ 16000000UL
#define CLK_TIMEOUT_MS 100UL
#define CLK_SW_WAIT_CYCLES 16000000UL
#define PLL_CLK_SW_WAIT_CYCLES 85UL
#define CLOCK_SOURCE (RCC->CFGR & RCC_CFGR_SWS_Msk)
#define PLL_SOURCE (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC_Msk)
#define PLLM ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM_Msk) >> RCC_PLLCFGR_PLLM_Pos)
#define PLLN ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN_Msk) >> RCC_PLLCFGR_PLLN_Pos)
#define PLLR ((RCC->PLLCFGR & RCC_PLLCFGR_PLLR_Msk) >> RCC_PLLCFGR_PLLR_Pos)
#define HPRE ((RCC->CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos)
#define PPRE1 ((RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos)
#define PPRE2 ((RCC->CFGR & RCC_CFGR_PPRE2_Msk) >> RCC_CFGR_PPRE2_Pos)
#define FLASH_WAIT_STATES (FLASH->ACR & FLASH_ACR_LATENCY_Msk)

static const uint32_t hpre_look_up[8] = {2, 4, 8, 16, 64, 128, 256, 512};

void NMI_Handler(void) {
    if (RCC->CIFR & RCC_CIFR_CSSF) {
        RCC->CICR = RCC_CICR_CSSC; // Clear Clock security system interrupt flag.
        fault_log(NMI_FAULT_FLAGS_REG, NMI_HSE_FAULT);
        systick_init(clock_hclk_hz());
    } else {
        fault_log(NMI_FAULT_FLAGS_REG, NMI_OTHER_FAULT);
        Error_Handler();
    }
}

static uint32_t get_hpre(void) { return HPRE > 7 ? hpre_look_up[HPRE - 8] : 1; }

// Switch system clock to clk_value, where clock_value can be (RCC_CFGR_SW_HSI / RCC_CFGR_SW_HSE /
// RCC_CFGR_SW_PLL).
static bool switch_system_clock(uint32_t clk_value) {
    RCC->CFGR = ((RCC->CFGR & ~(RCC_CFGR_SW_Msk)) |
                 (clk_value << RCC_CFGR_SW_Pos)); // Switch system-clock to clk_value.
    DWT->CYCCNT = 0;
    while (CLOCK_SOURCE != (clk_value << RCC_CFGR_SWS_Pos)) // While system-clock is NOT clk_value.
    {
        if (DWT->CYCCNT >=
            CLK_SW_WAIT_CYCLES) // 16 M cycles of waiting this would be atmost 1s if the clock is 16
                                // MHz and minimum ~94 ms if the clock is 170 MHz.
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
static void default_state(void) {
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
    uint32_t timestamp = 0; // Timestamp to be used when needed by waits

    systick_init(clock_hclk_hz()); // Init systick with the current HCLK.

    // Start the DWT cycle counter
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Start the power clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN_Msk;
    (void)RCC->APB1ENR1;

    /*
    Check if a software reset was executed on the last clock_init to avoid reset loop.
    No need to check if the system-clock is HSI, it should be HSI by default after reset.
    */
    if (fault_read(CLK_FAULT_FLAGS_REG) & (1 << CLK_RESET)) {
        fault_clear(CLK_FAULT_FLAGS_REG, CLK_RESET); // Clear RESET flag.
        default_state();                             // Set clock to default state
        systick_init(clock_hclk_hz());               // Init systick with the current HCLK.
        return CLK_SRC_HSI;
    }

    // Check if the PLL is already on before initiating.
    if (RCC->CR & RCC_CR_PLLON_Msk) {
        // PLL is on and must be turned off

        if (CLOCK_SOURCE != RCC_CFGR_SWS_HSI) { // Check if the system clock is running on 16 MHz
                                                // HSI. Switch to 16 MHz HSI if not running on HSI.
            if (switch_system_clock(RCC_CFGR_SW_HSI) ==
                false) // Switch system clock to HSI. If timeout
                       // log HSI_FAULT and RESET then software reset, otherwise continue.
            {
                fault_log(CLK_FAULT_FLAGS_REG, CLK_SW_HSI_FAULT); // Log HSI_FAULT flag.
                fault_log(CLK_FAULT_FLAGS_REG, CLK_RESET);        // Log RESET flag.
                fault_sw_restart();                               // Request software reset.
            }
        }

        systick_init(clock_hclk_hz()); // Init systick with current HCLK value to measure timeout.
        RCC->CR &= ~(RCC_CR_PLLON);    // Turn off PLL
        timestamp = systick_millis();
        while (RCC->CR & RCC_CR_PLLRDY_Msk) // While PLL locked, loop. If timed-out (after 100ms)
                                            // log PLL_OFF_FAULT and RESET then software reset.
        {
            if (((uint32_t)(systick_millis() - timestamp)) >= CLK_TIMEOUT_MS) {
                fault_log(CLK_FAULT_FLAGS_REG, CLK_PLL_OFF_FAULT); // Log PLL_OFF_FAULT flag.
                fault_log(CLK_FAULT_FLAGS_REG, CLK_RESET);         // Log RESET flag.
                fault_sw_restart();                                // Request software reset.
            }
        }
    }

    // HSE configuration
    RCC->CR |= RCC_CR_HSEON; // Turn the HSE on.
    timestamp = systick_millis();
    while (((RCC->CR & RCC_CR_HSERDY_Msk) != RCC_CR_HSERDY) &&
           (((uint32_t)(systick_millis() - timestamp)) <
            CLK_TIMEOUT_MS)) // While HSE crystal NOT ready and timeout not reached.
    {
        ;
    }
    if (RCC->CR & RCC_CR_HSERDY) { // If HSE is stable, set source to HSE.
        RCC->CR |= RCC_CR_CSSON;   // Set Clock Security System on.
        RCC->PLLCFGR = ((RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLSRC_Msk)) |
                        RCC_PLLCFGR_PLLSRC_HSE);               // Set PLL source to HSE;
        RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLM_Msk);               // Clear PLLM
        RCC->PLLCFGR |= ((6UL - 1UL) << RCC_PLLCFGR_PLLM_Pos); // Set PLLM = 6.
    } else { // If timedout, set PLL source to HSI and log HSE_FAULT. This will give the 170MHz
             // clock but at a worse accuracy of ±1%.
        fault_log(CLK_FAULT_FLAGS_REG, CLK_HSE_FAULT); // Log HSE_FAULT.
        RCC->CR &= ~(RCC_CR_HSEON);                    // Disable HSE.
        RCC->PLLCFGR = ((RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLSRC_Msk)) |
                        RCC_PLLCFGR_PLLSRC_HSI);               // Set PLL source to HSI;
        RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLM_Msk);               // Clear PLLM
        RCC->PLLCFGR |= ((4UL - 1UL) << RCC_PLLCFGR_PLLM_Pos); // Set PLLM = 4.
    }

    // PLL configuration
    RCC->PLLCFGR = ((RCC->PLLCFGR & ~(RCC_PLLCFGR_PLLN_Msk)) |
                    (85UL << RCC_PLLCFGR_PLLN_Pos)); // Set PLLN = 85.
    RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLR_Msk);         // Set PLLR = 2.
    RCC->CR |= RCC_CR_PLLON;                         // Turn on PLL.
    timestamp = systick_millis();
    while ((RCC->CR & RCC_CR_PLLRDY_Msk) !=
           RCC_CR_PLLRDY) // While PLL NOT locked, loop. If timed out (after 100ms)
                          // log PLL_FAULT then init default state and return.
    {
        if (((uint32_t)(systick_millis() - timestamp)) >= CLK_TIMEOUT_MS) {
            fault_log(CLK_FAULT_FLAGS_REG, CLK_PLL_FAULT); // Log PLL_FAULT flag.
            default_state();                               // Set clock to default state
            systick_init(clock_hclk_hz());                 // Init systick with the current HCLK.
            return CLK_SRC_HSI;
        }
    }
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN; // Enable PLLR output

    /*
    Switch from HSI to PLL. Steps in order are:
    1. Divide AHB by 2
    2. Clear R1MODE bit to enter boost mode
    3. Update wait states to 4
    4. Switch to PLL as source
    5. *After* clock switched, wait 1 us
    6. Configure AHB and other scalers as needed.
    */
    RCC->CFGR = ((RCC->CFGR & ~(RCC_CFGR_HPRE_Msk)) | RCC_CFGR_HPRE_DIV2); // Set AHB prescaler to 2
    RCC->CFGR &= ~(RCC_CFGR_PPRE2_Msk | RCC_CFGR_PPRE1_Msk); // Set APB1 and APB2 to not divided.
    (void)RCC->CFGR;                                         // Read back to confirm change.
    PWR->CR5 &= ~(PWR_CR5_R1MODE_Msk);                       // Set power to boost mode.
    (void)PWR->CR5;                                          // Read back to confirm change.
    FLASH->ACR =
        ((FLASH->ACR & ~(FLASH_ACR_LATENCY_Msk)) | FLASH_ACR_LATENCY_4WS); // Set wait stated to 4.
    (void)FLASH->ACR;                               // Read back to confirm values.
    if (FLASH_WAIT_STATES != FLASH_ACR_LATENCY_4WS) // Check to confirm wait states changed.
    {
        fault_log(CLK_FAULT_FLAGS_REG,
                  CLK_FLASH_WS_FAULT); // Set CLK_FLASH_WS_FAULT flag in CLK_FAULT_FLAGS_REG.
        default_state();               // Set clock to default state
        systick_init(clock_hclk_hz()); // Init systick with the current HCLK.
        return CLK_SRC_HSI;
    }
    if (switch_system_clock(RCC_CFGR_SW_PLL) ==
        false) // Switch system clock to PLL. If timeout then
               // log CLK_SW_FAULT and init default state, otherwise continue.
    {
        fault_log(CLK_FAULT_FLAGS_REG, CLK_SW_FAULT); // Log SW_FAULT flag.
        if (switch_system_clock(RCC_CFGR_SW_HSI) ==
            false) // Switch system clock to HSI. If timeout then
                   // log HSI_FAULT flag and RESET flag then software reset, otherwise continue.
        {
            fault_log(CLK_FAULT_FLAGS_REG, CLK_SW_HSI_FAULT); // Log SW_HSI_FAULT flag.
            fault_log(CLK_FAULT_FLAGS_REG, CLK_RESET);        // Log RESET flag.
            fault_sw_restart();                               // Software reset.
        }
        default_state();               // Set clock to default state
        systick_init(clock_hclk_hz()); // Init systick with the current HCLK.
        return CLK_SRC_HSI;
    }
    DWT->CYCCNT = 0;
    while (DWT->CYCCNT < PLL_CLK_SW_WAIT_CYCLES) // Must wait for 1us after PLL switch according to
                                                 // datasheet. at 85MHz 85 cycles is 1us.
    {
        ;
    }
    RCC->CFGR &=
        ~(RCC_CFGR_HPRE_Msk); // Set HPRE to not divide, giving the system clock the full 170MHz.
    (void)RCC->CFGR;          // Read back to confirm change.
    systick_init(clock_hclk_hz()); // Init systick with the current HCLK.

    return clock_source();
}

uint32_t clock_hclk_hz(void) {
    switch (CLOCK_SOURCE) {
    case RCC_CFGR_SWS_HSI: // If clock source is HSI return the HSI clock frequency divided by HPRE.
        return HSI_CLK_HZ / get_hpre();

    case RCC_CFGR_SWS_HSE: // If clock source is HSE return the HSE clock frequency divided by HPRE.
        return HSE_CLK_HZ / get_hpre();

    case RCC_CFGR_SWS_PLL: // If clock source is PLL calculate using '(PLLSOURCE/PLLM)*PLLN / PLLR /
                           // HPRE' then return PLL frequency.
    {
        uint32_t clock_frequency = 0;
        if (PLL_SOURCE == RCC_PLLCFGR_PLLSRC_HSE) {
            clock_frequency = HSE_CLK_HZ;
        } else if (PLL_SOURCE == RCC_PLLCFGR_PLLSRC_HSI) {
            clock_frequency = HSI_CLK_HZ;
        }
        clock_frequency /= PLLM + 1;       // Divide by PLLM
        clock_frequency *= PLLN;           // Multiply by PLLN
        clock_frequency /= 2 + (PLLR * 2); // Divide by PLLR
        clock_frequency /= get_hpre();     // Divide by AHB Prescaler HPRE
        return clock_frequency;
    }
    default:
        Error_Handler(); // Impossible state.
    }
}

uint32_t clock_pclk1_hz(void) {
    uint32_t hclk = clock_hclk_hz();
    if (PPRE1 < 4) {
        return hclk;
    }
    uint32_t apb1_prescaler = 1 << (PPRE1 - 3);
    return hclk / apb1_prescaler;
}

uint32_t clock_pclk2_hz(void) {
    uint32_t hclk = clock_hclk_hz();
    if (PPRE2 < 4) {
        return hclk;
    }
    uint32_t apb2_prescaler = 1 << (PPRE2 - 3);
    return hclk / apb2_prescaler;
}

clk_source clock_source(void) {
    switch (CLOCK_SOURCE) {
    case RCC_CFGR_SWS_HSI:
        return CLK_SRC_HSI;

    case RCC_CFGR_SWS_HSE:
        return CLK_SRC_HSE;

    case RCC_CFGR_SWS_PLL:
        if (PLL_SOURCE == RCC_PLLCFGR_PLLSRC_HSI) {
            return CLK_SRC_HSI_PLL;
        }
        return CLK_SRC_HSE_PLL;

    default:
        Error_Handler(); // Impossible state.
    }
}