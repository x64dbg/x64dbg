SECTIONS
{
    . = SIZEOF_HEADERS;
    .note.gnu.property : { *(.note.gnu.property) }
    .note.gnu.build-id : { *(.note.gnu.build-id) }
    .hash : { *(.hash) }
    .gnu.hash : { *(.gnu.hash) }
    .dynsym : { *(.dynsym) }
    .dynstr : { *(.dynstr) }
    .gnu.version : { *(.gnu.version) }
    .gnu.version_d : { *(.gnu.version_d) }
    .gnu.version_r : { *(.gnu.version_r) }
    .rela.dyn : { *(.rela.*) }
    .rodata : { *(.rodata .rodata.*) }
    . = ALIGN(CONSTANT(MAXPAGESIZE));
    .dynamic : { *(.dynamic) }
    .got : { *(.got .got.plt) }
    .data : { *(.data .data.*) }
    .bss : { *(.bss .bss.*) }
}
