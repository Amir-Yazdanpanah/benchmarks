.section .text.init
.globl _start
_start:
    # Only hart 0 owns the workload and output buffers.
    csrr t0, mhartid
    bnez t0, .Lhang
    la sp, _stack_top
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    # FS=Dirty (bits 14:13); safe with the original RV64IMAFD target.
    li t0, 0x6000
    csrs mstatus, t0

    # Raw binaries do not include the NOLOAD BSS section.
    la t0, __bss_start
    la t1, __bss_end
.Lzero:
    bgeu t0, t1, .Lmain
    sd zero, 0(t0)
    addi t0, t0, 8
    j .Lzero
.Lmain:
    call main
.Lhang:
    j .Lhang
