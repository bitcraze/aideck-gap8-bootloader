OUTPUT_ARCH(riscv)
ENTRY( _start )
MEMORY
{
  L2                 : ORIGIN = 0x1C060000, LENGTH = 0x00018000
  fc_tcdm            : ORIGIN = 0x1b002000, LENGTH = 0x00001000
}

SECTIONS
{
  .stack : {
    . = ALIGN(4);
    *(.data_fc)
    *(.data_fc.*)
    *(.fcTcdm)
    *(.fcTcdm.*)
    *(.data_tiny_fc)
    *(.data_tiny_fc.*)
    *(.fcTcdm_g)
    *(.fcTcdm_g.*)
    . = ALIGN(16);
  } > fc_tcdm

  __fc_tcdm_end = ALIGN(4);



  .vectors :
  {
    __irq_vector_base = .;
    KEEP(*(.vectors))
  } > L2

  .text :
  {
    . = ALIGN(4);
    _stext = .;
    *(.text)
    *(.text.*)
    . = ALIGN(4);
    __cluster_text_start = .;
    *(.cluster.text)
    *(.cluster.text.*)
    __cluster_text_end = .;
    . = ALIGN(4);
    _etext  =  .;
    *(.lit)
    *(.shdata)
    _endtext = .;
    . = ALIGN(4);
  } > L2


  /* The following data are kept in L2 because of a HW bug which makes
   * byte acceses failing from cluster to FC TCDM
   */

  .init_array : {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    __CTOR_LIST__ = .;
    LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
    KEEP(*(.ctors.start))
    KEEP(*(.ctors))
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array ))
    LONG(0)
    __CTOR_END__ = .;
    PROVIDE_HIDDEN (__init_array_end = .);
  } > L2


  .fini_array : {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    __DTOR_LIST__ = .;
    LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
    KEEP(*(.dtors.start))
    KEEP(*(.dtors))
    LONG(0)
    __DTOR_END__ = .;
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array ))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > L2

  .init :
  {
    . = ALIGN(4);
    KEEP( *(.init) )
  } > L2


  .fini :
  {
    . = ALIGN(4);
    KEEP( *(.fini) )
  } > L2


  .preinit_array : {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > L2



  .boot : {
    . = ALIGN(4);
    *(.boot)
    *(.boot.data)
  } > L2



  .got : {
    . = ALIGN(4);
    *(.got.plt) * (.igot.plt) *(.got) *(.igot)
  } > L2


  .shbss : {
    . = ALIGN(4);
    *(.shbss)
  } > L2


  .talias : {
  } > L2


  .gnu.offload_funcs : {
    . = ALIGN(4);
    KEEP(*(.gnu.offload_funcs))
  } > L2


  .gnu.offload_vars : {
    . = ALIGN(4);
    KEEP(*(.gnu.offload_vars))
  } > L2


  .rodata : {
    . = ALIGN(4);
    *(.rodata);
    *(.rodata.*)
    *(.srodata);
    *(.srodata.*)
    *(.eh_frame*)
  } > L2

  .data : {
    . = ALIGN(4);
    sdata  =  .;
    _sdata  =  .;
    *(.data);
    *(.data.*)
    *(.sdata);
    *(.sdata.*)
    *(.heapl2ram)
    *(.l2_data)
    *(.l2_data.*)
    . = ALIGN(4);
    edata  =  .;
    _edata  =  .;
  } > L2


  .bss : {
    . = ALIGN(8);
    _bss_start = .;
    *(.bss)
    *(.bss.*)
    *(.sbss)
    *(.sbss.*)
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
    __l2_end = .;
  } > L2

  _l1_preload_start = 0;
  _l1_preload_start_inL2 = 0;
  __l1_end = 0;
  _l1_preload_size = 0;
  __cluster_text_size = 0;
  __l1_heap_start = 0;
  __l1_heap_size = 0;
}