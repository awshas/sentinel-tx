.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb
.global g_pfnVectors
.global Default_Handler
.global Reset_Handler
.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    .size g_pfnVectors, .-g_pfnVectors
.section .text.Default_Handler,"ax",%progbits
.type Default_Handler, %function
Default_Handler:
    b .
.size Default_Handler, .-Default_Handler
.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler
.weak MemManage_Handler
.thumb_set MemManage_Handler, Default_Handler
.weak BusFault_Handler
.thumb_set BusFault_Handler, Default_Handler
.weak UsageFault_Handler
.thumb_set UsageFault_Handler, Default_Handler
.weak SVC_Handler                             
.thumb_set SVC_Handler, Default_Handler
.weak DebugMon_Handler                        
.thumb_set DebugMon_Handler, Default_Handler  
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler

.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
Copy_Data:
    cmp r2, r1
    bls Copy_Data_Done
    ldr r3, [r0], #4
    str r3, [r1], #4
    b Copy_Data
Copy_Data_Done:
@ Write 0's to .bss
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, 0
Write_0:
    cmp r1, r0
    bls Write_0_bss_Done
    str r2, [r0], #4
    b Write_0
Write_0_bss_Done:
    bl __libc_init_array
    bl main
    b .                      @ main should never return; trap if it does
.size Reset_Handler, .-Reset_Handler