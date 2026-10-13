OUTPUT_ARCH(riscv)
ENTRY(_start)
MEMORY { RAM : ORIGIN = 0x80000000, LENGTH = 16M }
SECTIONS {
    . = ORIGIN(RAM);
    .text : {
        KEEP(*(.text.init))
        *(.text .text.*)
    } > RAM
    .rodata : { *(.rodata .rodata.* .srodata .srodata.*) } > RAM
    .data : {
        *(.data .data.*)
        PROVIDE(__global_pointer$ = . + 0x800);
        *(.sdata .sdata.*)
    } > RAM
    .bss (NOLOAD) : {
        . = ALIGN(8);
        __bss_start = .;
        *(.bss .bss.* .sbss .sbss.*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    } > RAM
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
    _stack_bottom = _stack_top - 0x4000;
    ASSERT(__bss_end <= _stack_bottom, "Program overlaps the 16 KiB stack")
    ASSERT(_start == ORIGIN(RAM), "Raw binary must start with _start")
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
