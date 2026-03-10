# zsbl.S — Zero Stage Boot Loader
# Runs in M-mode at 0x80000000
# 1. Sets up M-mode stack
# 2. Zeroes BSS
# 3. Installs M-mode trap vector (handles SBI ecall + M-timer)
# 4. Configures PMP
# 5. Delegates exceptions/interrupts to S-mode
# 6. Enables M-mode timer interrupt
# 7. Drops to S-mode via mret

.section .text.zsbl
.global _start

_start:
    # Set up M-mode stack
    la      sp, _mstack_top

    # Zero BSS
    la      t0, _bss_start
    la      t1, _bss_end
zero_bss:
    beq     t0, t1, bss_done
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       zero_bss
bss_done:

    # Print "ZSBL\r\n" to UART — confirms M-mode is alive
    li      t0, 0x10000000
    li      t1, 'Z'
    sb      t1, 0(t0)
    li      t1, 'S'
    sb      t1, 0(t0)
    li      t1, 'B'
    sb      t1, 0(t0)
    li      t1, 'L'
    sb      t1, 0(t0)
    li      t1, '\r'
    sb      t1, 0(t0)
    li      t1, '\n'
    sb      t1, 0(t0)

    # Install M-mode trap vector
    la      t0, _mtrap_vector
    csrw    mtvec, t0

    # PMP: grant S-mode full access to all memory
    li      t0, 0x3FFFFFFFFFFFFFFF
    csrw    pmpaddr0, t0
    li      t0, 0x0F                # TOR | R | W | X
    csrw    pmpcfg0, t0

    # Delegate all standard exceptions to S-mode
    # except S-mode ecall (bit 9) — keep in M-mode for SBI
    li      t0, 0xFFFF
    li      t1, (1 << 9)
    not     t1, t1
    and     t0, t0, t1
    csrw    medeleg, t0

    # Delegate S-mode software (bit 1), timer (bit 5),
    # and external (bit 9) interrupts to S-mode.
    # CRITICAL: bit 5 must be set — without it sie.STIE is
    # hardwired to 0 and S-mode can never enable timer interrupts.
    li      t0, 0x0222
    csrw    mideleg, t0

    # Enable M-mode timer interrupt (MTIE, bit 7) in mie
    # M-mode trap fires when mtime >= mtimecmp,
    # then we inject STIP into S-mode via mip
    li      t0, (1 << 7)
    csrw    mie, t0

    # Enable M-mode global interrupts (MIE, bit 3) in mstatus
    csrr    t0, mstatus
    ori     t0, t0, (1 << 3)
    csrw    mstatus, t0

    # Set mepc = supervisor entry at 0x80200000
    li      t0, 0x80200000
    csrw    mepc, t0

    # Set mstatus.MPP = 01 (S-mode)
    csrr    t0, mstatus
    li      t1, ~(3 << 11)          # clear MPP [12:11]
    and     t0, t0, t1
    li      t1, (1 << 11)           # MPP = 01 = Supervisor
    or      t0, t0, t1
    csrw    mstatus, t0

    # Disable virtual memory
    csrw    satp, zero

    # a0 = hartid, a1 = DTB — preserved from QEMU reset vector
    mret


# ════════════════════════════════════════════════════════
# M-mode trap vector
# Handles:
#   mcause = INT|7  : M-mode timer  → inject STIP to S-mode
#   mcause = 9      : S-mode ecall  → SBI SET_TIMER (ext 0x00)
# ════════════════════════════════════════════════════════
.align 2
_mtrap_vector:

    # Save working registers on M-mode stack
    addi    sp, sp, -32
    sd      t0,  0(sp)
    sd      t1,  8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)

    csrr    t0, mcause

    # Check for M-mode timer interrupt (bit63=1, code=7)
    li      t1, 7
    li      t2, 1
    slli    t2, t2, 63
    or      t1, t1, t2              # t1 = 0x8000000000000007
    beq     t0, t1, _mtimer_irq

    # Check for S-mode ecall (code = 9)
    li      t1, 9
    beq     t0, t1, _sbi_ecall

    # Unknown trap — hang so debugger can inspect
_trap_hang:
    csrr    t0, mcause
    csrr    t1, mepc
    csrr    t2, mtval
1:  j       1b


# ── M-mode timer interrupt handler ───────────────────────
# mtime >= mtimecmp fired in M-mode.
# Disable MTIE, then set STIP in mip so S-mode sees the interrupt.
_mtimer_irq:
    # Clear MTIE (bit 7) in mie — stop M-mode timer from re-firing
    li      t0, ~(1 << 7)
    csrr    t1, mie
    and     t1, t1, t0
    csrw    mie, t1

    # Set STIP (bit 5) in mip — inject supervisor timer pending
    li      t0, (1 << 5)
    csrs    mip, t0

    ld      t0,  0(sp)
    ld      t1,  8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    addi    sp, sp, 32
    mret


# ── SBI ecall handler ─────────────────────────────────────
# S-mode called ecall with:
#   a7 = SBI extension ID (0x00 = legacy SET_TIMER)
#   a0 = absolute mtime value for next interrupt
_sbi_ecall:
    # Only handle extension 0x00 (SET_TIMER)
    li      t0, 0
    bne     a7, t0, _sbi_unsupported

    # Write mtimecmp for hart 0 at CLINT 0x2004000
    li      t0, 0x2004000
    sd      a0, 0(t0)

    # Clear STIP (bit 5) in mip — timer no longer pending
    li      t0, ~(1 << 5)
    csrr    t1, mip
    and     t1, t1, t0
    csrw    mip, t1

    # Re-enable MTIE (bit 7) in mie — arm M-mode for next tick
    li      t0, (1 << 7)
    csrs    mie, t0

    # SBI return values
    li      a0, 0                   # error code = 0 (success)
    li      a1, 0                   # value = 0
    j       _sbi_done

_sbi_unsupported:
    li      a0, -1                  # error code = -1
    li      a1, 0

_sbi_done:
    # Advance mepc past the ecall instruction
    csrr    t0, mepc
    addi    t0, t0, 4
    csrw    mepc, t0

    ld      t0,  0(sp)
    ld      t1,  8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    addi    sp, sp, 32
    mret


# ── M-mode stack (4 KB) ──────────────────────────────────
.section .mstack
.align 12
_mstack_bottom:
    .space  4096
_mstack_top:
