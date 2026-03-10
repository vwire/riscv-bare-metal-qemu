# RISC-V Bare-Metal on QEMU

Two self-contained bare-metal RISC-V projects that run on **QEMU virt**.  
No OS. No OpenSBI dependency (for Project 1). Fully debuggable in **Eclipse CDT**.

📖 **Documentation site:** https://YOUR_USERNAME.github.io/riscv-bare-metal-qemu

---

## Projects

| | Project | Description | Privilege |
|---|---|---|---|
| 1 | [`bare_metal_qemu/`](bare_metal_qemu/) | Minimal RV64I assembly — stack, arithmetic, loop | M-mode |
| 2 | [`zsbl_supervisor/`](zsbl_supervisor/) | Two-stage boot: M-mode ZSBL + S-mode C supervisor with SBI timer | M → S-mode |

---

## Quick Start

```bash
# Prerequisites
sudo apt install qemu-system-misc gdb-multiarch make
# RISC-V toolchain: https://github.com/sifive/freedom-tools/releases

# Clone
git clone https://github.com/YOUR_USERNAME/riscv-bare-metal-qemu.git
cd riscv-bare-metal-qemu

# Project 1 — bare metal
cd bare_metal_qemu && make qemu-run
# (loops silently — no UART output, Ctrl-A X to quit)

# Project 2 — ZSBL + Supervisor
cd ../zsbl_supervisor && make qemu-run
```

**Expected output for Project 2:**
```
ZSBL
########################################
#   RISC-V ZSBL + Supervisor Demo      #
########################################
  Hart ID : 0x0000000000000000
  DTB     : 0x0000000087E00000
[---- Arithmetic Demo ----]
[---- Memory Read/Write Demo ----]
  Result: 4/4 passed
[---- Timer Interrupt Demo ----]
  Waiting for 3 interrupts...
[IRQ] Timer #1  mtime=0x...
[IRQ] Timer #2  mtime=0x...
[IRQ] Timer #3  mtime=0x...
  3 timer interrupts received.
########################################
#   Demo complete. System halted.       #
########################################
```

---

## Repository Structure

```
riscv-bare-metal-qemu/
├── bare_metal_qemu/
│   ├── main.S          # RV64I assembly program
│   ├── linker.ld       # places .text at 0x80000000
│   └── Makefile
├── zsbl_supervisor/
│   ├── zsbl.S              # M-mode Zero Stage Boot Loader
│   ├── supervisor_entry.S  # S-mode entry + trap vector
│   ├── supervisor.c        # S-mode C supervisor
│   ├── supervisor.h
│   ├── linker_zsbl.ld      # ZSBL at 0x80000000
│   ├── linker_super.ld     # Supervisor at 0x80200000
│   └── Makefile
└── docs/                   # GitHub Pages documentation site
    ├── index.html          # landing page + prerequisites
    ├── project1.html       # Project 1 complete guide
    ├── project2.html       # Project 2 complete guide
    └── reset-vector.html   # QEMU reset vector deep dive
```

---

## Makefile Targets

Both projects share the same targets:

| Target | Action |
|---|---|
| `make` or `make all` | Build ELF(s) |
| `make qemu-run` | Build and run in QEMU (no debugger) |
| `make qemu` | Build and run paused — waiting for GDB on port 1234 |
| `make clean` | Remove build artefacts |

---

## Eclipse CDT Debug

Full step-by-step Eclipse setup is in the documentation pages.  
Short version: use **GDB Hardware Debugging**, set GDB to `/usr/bin/gdb-multiarch`, host `localhost`, port `1234`.

---

## Key Concepts Covered

- RISC-V privilege levels (M-mode, S-mode)
- QEMU virt reset vector (5 instructions at 0x1000)
- Bare-metal linker scripts and ELF entry points
- Machine-mode trap handling (`mtvec`, `mcause`, `mret`)
- Physical Memory Protection (PMP)
- Interrupt delegation (`medeleg`, `mideleg`)
- SBI (Supervisor Binary Interface) — SET_TIMER ecall
- CLINT timer (`mtime`, `mtimecmp`, STIP injection)
- S-mode trap vector (`stvec`, `sstatus`, `sret`)
- Eclipse CDT GDB Hardware Debugging

---

## Toolchain

Tested with:
- `riscv64-unknown-elf-as` / `ld` — GNU binutils 2.35+
- `riscv64-unknown-elf-gcc` — GCC 10.2
- `qemu-system-riscv64` — QEMU 6.x / 8.x
- `gdb-multiarch` — GDB 12+
- Eclipse CDT 11+
