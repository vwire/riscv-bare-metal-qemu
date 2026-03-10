/* linker_zsbl.ld — ZSBL memory map
 * Places ZSBL at 0x80000000 (QEMU virt RAM base)
 */

ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 2M
}

SECTIONS
{
    . = 0x80000000;

    .text : {
        *(.text.zsbl)   /* _start must be first */
        *(.text*)
    } > RAM

    .rodata : { *(.rodata*) } > RAM

    .data : { *(.data*) } > RAM

    .bss : {
        _bss_start = .;
        *(.bss*) *(COMMON)
        _bss_end = .;
    } > RAM

    .mstack : { *(.mstack) } > RAM
}
