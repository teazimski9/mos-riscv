.section .text.init
.globl _start
.type _start, @function

_start:

    csrr a0, mhartid
    bnez a0, .Lpark

    la sp, stack_top

    li t0, 0x2000
    csrs mstatus, t0

    call main

.Lpark:

    wfi
    j .Lpark
