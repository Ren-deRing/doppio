ENTRY(boot_entry)

PHDRS
{
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
}

SECTIONS
{
    . = 0xffffffff80000000;
    
    __kernel_start = .;
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        __text_start = .;
        *(.text .text.*)
        __text_end = .;
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        __rodata_start = .;
        *(.rodata .rodata.*)

        /* Initcall */
        . = ALIGN(8);
        __initcall_start = .;
        KEEP(*(.initcall.0*)) /* Early */
        KEEP(*(.initcall.1*)) /* L0: Arch 초기화 */
        KEEP(*(.initcall.2*)) /* L1: Memory 초기화 */
        KEEP(*(.initcall.3*)) /* L2: Bus/Driver 초기화 */
        KEEP(*(.initcall.4*)) /* L3: Subsytem 초기화 */
        KEEP(*(.initcall.5*)) /* L4: Userland 준비 */
        __initcall_end = .;

        __rodata_end = .;
    } :rodata

    .rela.dyn : {
        __rela_start = .;
        *(.rela.dyn)
        *(.rela.*)
        __rela_end = .;
    } :rodata

    .note.gnu.build-id : {
        *(.note.gnu.build-id)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } :data

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    } :data

    . = ALIGN(CONSTANT(MAXPAGESIZE));
    __kernel_end = .;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}