# main.S — Bare-metal RV64I assembly on QEMU virt
# Demonstrates: stack setup, arithmetic, loops.
# Load address: 0x80000000  (QEMU virt DRAM base)
# Debug with:   make qemu  then connect Eclipse/GDB on :1234

.section .text
.global _start

_start:
    # ── Stack setup ──────────────────────────────────────
    # Point sp to top of a 64 KB scratch area.
    # Must be done before any call instruction.
    li      sp, 0x80010000

    call    main

halt:
    j       halt          # spin here if main ever returns

# ── main ─────────────────────────────────────────────────
.global main
main:
    # Basic arithmetic
    li      a0, 10        # a0  = 10
    li      a1, 20        # a1  = 20
    add     a2, a0, a1    # a2  = 30  (10 + 20)

    # Count loop: t0 = 0 → 4  (5 iterations)
    li      t0, 0
    li      t1, 5
loop:
    addi    t0, t0, 1
    blt     t0, t1, loop  # branch if t0 < 5

done:
    j       done          # set breakpoint here in Eclipse
