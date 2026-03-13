/* linker.ld — OpenSBI payload at 0x80200000
 * OpenSBI fw_jump jumps here after M-mode init
 */

ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80200000, LENGTH = 2M
}

SECTIONS
{
    . = 0x80200000;

    .text : {
        *(.text.entry)      /* _start must be first */
        *(.text*)
    } > RAM

    .rodata : { *(.rodata*) } > RAM
    .data   : { *(.data*)   } > RAM

    .bss : {
        _bss_start = .;
        *(.bss*) *(COMMON)
        _bss_end = .;
    } > RAM

    .sstack : { *(.sstack) } > RAM
}
