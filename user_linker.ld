SECTIONS {
    . = 0x400000;

    .text : {
        *(.entry*)
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
    }
}