# entry.S — S-mode entry point at 0x80200000
# OpenSBI jumps here after M-mode init with:
#   a0 = hartid
#   a1 = DTB address

.section .text.entry
.global _start

#_start:
#    li      sp, 0x80203000          # top of 8KB stack at 0x80201000
#    la      t0, _strap_vector
#    csrw    stvec, t0
#    call    payload_main
##    la      sp, _sstack_top         # set up S-mode stack
#    la      t0, _strap_vector
#    csrw    stvec, t0               # install S-mode trap vector
#    call    payload_main            # a0=hartid, a1=dtb
#
#1:  wfi
#    j       1b
_start:
    lui     sp, 0x80
    addiw   sp, sp, 515
    slli    sp, sp, 12

    li      t0, 0x10000000
    li      t1, 0x41
    sb      t1, 0(t0)             # print 'A' — proof _start ran

    la      t0, _strap_vector
    csrw    stvec, t0
    call    payload_main

1:  wfi
    j       1b

# ── S-mode trap vector ────────────────────────────────────
.align 2
.global _strap_vector
_strap_vector:
    addi    sp, sp, -256
    sd      ra,   0(sp)
    sd      t0,   8(sp)
    sd      t1,  16(sp)
    sd      t2,  24(sp)
    sd      a0,  32(sp)
    sd      a1,  40(sp)
    sd      a2,  48(sp)
    sd      a3,  56(sp)
    sd      a4,  64(sp)
    sd      a5,  72(sp)
    sd      a6,  80(sp)
    sd      a7,  88(sp)
    sd      t3,  96(sp)
    sd      t4, 104(sp)
    sd      t5, 112(sp)
    sd      t6, 120(sp)

    call    payload_trap_handler

    ld      ra,   0(sp)
    ld      t0,   8(sp)
    ld      t1,  16(sp)
    ld      t2,  24(sp)
    ld      a0,  32(sp)
    ld      a1,  40(sp)
    ld      a2,  48(sp)
    ld      a3,  56(sp)
    ld      a4,  64(sp)
    ld      a5,  72(sp)
    ld      a6,  80(sp)
    ld      a7,  88(sp)
    ld      t3,  96(sp)
    ld      t4, 104(sp)
    ld      t5, 112(sp)
    ld      t6, 120(sp)
    addi    sp, sp, 256
    sret

# ── S-mode stack (8 KB) ──────────────────────────────────
.section .sstack
.align 12
_sstack_bottom:
    .space  8192
_sstack_top:
