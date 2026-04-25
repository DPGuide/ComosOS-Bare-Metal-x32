ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS {
    . = 1M;

    .text : ALIGN(4K) {
        /* Wir erzwingen, dass boot.o (mit dem Header) GANZ vorne steht */
        boot.o(.multiboot)
        boot.o(.text)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}