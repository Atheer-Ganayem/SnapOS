ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS
{
  . = 0xFFFFFFFF81000000;

  .text ALIGN(4k) : {
    *(.entry)
    *(.text)
    *(.text.*)
  }

  .rodata ALIGN(4k) : {
    *(.rodata)
    *(.rodata.*)
  }

  .data ALIGN(4k) : {
    *(.data)
    *(.data.*)
  }

  .bss ALIGN(4K) : {
    _bss_start = .;
    *(COMMON)
    *(.bss)
    *(.bss.*)
    _bss_end = .;
  }
  _kernel_end = .;
}