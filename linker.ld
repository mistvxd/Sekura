ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);    /* R + X */
    rodata PT_LOAD FLAGS(4);  /* R */
    data PT_LOAD FLAGS(6);    /* R + W */
}

SECTIONS
{
    . = 0xffffffff80000000;

    . = ALIGN(4096);
    .text :
    {
        *(.text*)
    } :text

    . = ALIGN(4096);
    .rodata :
    {
        *(.rodata*)
    } :rodata

    . = ALIGN(4096);
    .data :
    {
        *(.data*)
    } :data

    . = ALIGN(4096);
    .bss (NOLOAD) :
    {
        *(COMMON)
        *(.bss*)
    } :data
}