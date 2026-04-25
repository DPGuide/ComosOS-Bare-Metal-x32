ENTRY(os2_start)
SECTIONS
{
    . = 0x1000000;
    .text : ALIGN(4096) {
        *(.text.entry)
        *(.text .text.*)
    }
    .rodata : ALIGN(4096) { *(.rodata .rodata.*) }
    .data : ALIGN(4096) { *(.data .data.*) }
    .bss : ALIGN(4096) { *(COMMON) *(.bss .bss.*) }
}
