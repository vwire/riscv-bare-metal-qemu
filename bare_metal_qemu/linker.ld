/* linker.ld — bare-metal single-binary layout
 * Everything placed at DRAM base 0x80000000.
 */

ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
    .text 0x80000000 : {
        *(.text)
        *(.text.*)
    } > RAM

    .data : { *(.data) *(.data.*) } > RAM
    .bss  : { *(.bss)  *(.bss.*)  } > RAM
}
