PROVIDE(_Min_Stack_Size = 0x400);   /* overridable via -Wl,--defsym */
MEMORY   {      /* what memory exists — the physical map */
    FLASH   (rx)    : ORIGIN = 0x08000000, LENGTH = 512K
    RAM     (rwx)   : ORIGIN = 0x20000000, LENGTH = 96K
    CCMRAM  (rwx)    : ORIGIN = 0x10000000, LENGTH = 32K     /* The ORIGIN of CCMRAM is 0x1000 0000 but that is only accessed by the CPU. It is alliased at 0x2001 8000 for DMA access.*/
}
ENTRY(Reset_Handler)              /* the symbol execution starts at */
_estack = ORIGIN(RAM) + LENGTH(RAM);   /* initial stack pointer */

SECTIONS
{
    .isr_vector : ALIGN(512); /*VTOR requires the table to be aligned to the next power of two ≥ (number of vectors × 4). The G474 vector table is 16 system exceptions plus ~102 IRQs ≈ 118 entries × 4 = ~472 bytes, rounding to 512-byte alignment.*/
    {
        KEEP(*(.isr_vector))
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
    } > FLASH
    
    . = ALIGN(4);
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx*)
    } > FLASH
    . = ALIGN(4);
    __exidx_end = .;

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