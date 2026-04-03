/* Sagt dem Linker, dass wir eine 64-Bit ELF-Datei für x86_64 bauen wollen */
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

/* Dein Einsprungspunkt (musst du anpassen, falls deine main-Funktion anders heißt, z.B. _start) */
ENTRY(kernel_main)

PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    . = 0xffffffff80000000;

    .text : {
        *(.text .text.*)
    }

    /* Diese Sektion ist für Limine lebenswichtig */
    .rodata : {
        *(.rodata .rodata.*)
        *(.requests) 
    }

    .data : {
        *(.data .data.*)
    }

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
}