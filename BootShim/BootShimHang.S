// Diagnostic HANG shim for headless bring-up. NOT a production entry path.
//
// Purpose: answer two questions with no dependence on the display pipeline
// being correct:
//
//   1. Does the bootloader actually jump to our image at all?  If it does,
//      we disable the MediaTek watchdog and spin forever. A device that
//      stops rebooting (no more USB re-enumeration) proves execution.
//   2. Is FB_BASE the buffer the panel is actually scanning?  Before hanging
//      we paint the top FB_FILL bytes white (RGB/BGR-agnostic). If FB_BASE is
//      right the panel turns white while hung; otherwise the stock logo stays.
//
// The ARM64 header must match BootShim.S (text_offset = FD_BASE) so the
// bootloader's load path accepts the blob exactly like the real shim.

_Head:
  adr   x1, _Payload
  b     _Start

#if REQUIRES_KERNEL_HEADER == 1
_StackBase:
  .quad FD_BASE         /* Text Offset */
_StackSize:
  .quad FD_SIZE + (_Payload - _Head)
  .quad 0               /* Flags */
  .quad 0               /* Reserved */
  .quad 0               /* Reserved */
  .quad 0               /* Reserved */
  .ascii "ARM\x64"      /* ARM64 Magic */
  .long 0               /* Reserved */
#endif

_Start:
  /* --- q2: paint the scan-out candidate white and flush it to DRAM --- */
  ldr   x0, =FB_BASE
  ldr   x2, =FB_FILL
  mov   x3, #-1
_Paint:
  stp   x3, x3, [x0], #16
  subs  x2, x2, #16
  b.ne  _Paint
  mrs   x9, ctr_el0
  ubfx  x9, x9, #16, #4
  mov   x10, #4
  lsl   x9, x10, x9
  sub   x10, x9, #1
  ldr   x0, =FB_BASE
  bic   x8, x0, x10
  ldr   x7, =FB_FILL
  add   x7, x0, x7
_ProbeClean:
  dc    cvac, x8
  add   x8, x8, x9
  cmp   x8, x7
  b.lo  _ProbeClean
  dsb   sy

  /* --- q1: disable the MediaTek watchdog, then spin forever --- */
  ldr   x0, =WD_BASE
  ldr   w1, =WD_KEY
  str   w1, [x0]
  dsb   sy
_Hang:
  wfi
  b     _Hang

.text
.ltorg
.align 4
_Payload:
