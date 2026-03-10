/* linker_super.ld — Supervisor memory map
 * Places supervisor at 0x80200000 (2MB above ZSBL)
 */

ENTRY(_supervisor_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80200000, LENGTH = 2M
}

SECTIONS
{
    . = 0x80200000;

    .text : {
        *(.text.super_entry)  /* _supervisor_start must be first */
        *(.text*)
    } > RAM

    .rodata : { *(.rodata*) } > RAM

    .data : { *(.data*) } > RAM

    .bss : {
        _sbss_start = .;
        *(.bss*) *(COMMON)
        _sbss_end = .;
    } > RAM

    .sstack : { *(.sstack) } > RAM
}
