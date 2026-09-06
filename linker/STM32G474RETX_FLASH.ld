PROVIDE(_Min_Stack_Size = 0x400);   /* overridable via -Wl,--defsym */
MEMORY   {      /* what memory exists — the physical map */
    FLASH   (rx)    : ORIGIN = 0x08000000, LENGTH = 512K
    RAM     (rwx)   : ORIGIN = 0x20000000, LENGTH = 96K
    CCMRAM  (rwx)    : ORIGIN = 0x10000000, LENGTH = 32K     
    /* CCM SRAM has two mappings: 0x10000000 (CPU only, via I-Code/D-Code) and
   0x20018000 (all masters, via SBUS). We use the CPU-side mapping: the
   alias is DMA-reachable but reached over SBUS, forfeiting the bus-matrix
   isolation that is the entire point of CCM. DMA buffers belong in RAM. */
}
ENTRY(Reset_Handler)              /* the symbol execution starts at */
_estack = ORIGIN(RAM) + LENGTH(RAM);   /* initial stack pointer */

SECTIONS
{
    /*VTOR requires the table to be aligned to the next power of two ≥ (number of vectors × 4). The G474 vector table is 16 system exceptions plus ~102 IRQs ≈ 118 entries × 4 = ~472 bytes, rounding to 512-byte alignment.*/
    .isr_vector : ALIGN(512)
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata*)
    } > FLASH
    
    . = ALIGN(4);
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > FLASH
    . = ALIGN(4);
    __exidx_end = .;

        .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH
    
    _sidata = LOADADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .user_heap_stack :
    {
        . = ALIGN(8);
        . = . + _Min_Stack_Size;
    } > RAM

    /DISCARD/ :
    {
        *(.comment*)
        *(.ARM.attributes*)
    }
}