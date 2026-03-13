# RISC-V Bare-Metal on QEMU

Three self-contained bare-metal RISC-V projects that run on **QEMU virt**.  
No OS. Fully debuggable in **Eclipse CDT**.

📖 **Documentation site:** <https://vwire.github.io/riscv-bare-metal-qemu>

---

## Projects

|  | Project | Description | Privilege |
|--|---------|-------------|-----------|
| 1 | [`bare_metal_qemu/`](bare_metal_qemu/) | Minimal RV64I assembly — stack, arithmetic, loop | M-mode |
| 2 | [`zsbl_supervisor/`](zsbl_supervisor/) | Two-stage boot: M-mode ZSBL + S-mode C supervisor with legacy SBI timer | M → S-mode |
| 3 | [`opensbi_payload/`](opensbi_payload/) | Real OpenSBI from source + S-mode payload using SBI v1.0 TIME ext and sstc `stimecmp` | S-mode under OpenSBI |

---

## Quick Start

```bash
# Prerequisites
sudo apt install qemu-system-misc gdb-multiarch make
# Bare-metal toolchain: https://github.com/sifive/freedom-tools/releases
# Linux RISC-V toolchain (Project 3 only):
sudo apt install gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu

# Clone
git clone https://github.com/vwire/riscv-bare-metal-qemu.git
cd riscv-bare-metal-qemu

# Project 1 — bare metal
cd bare_metal_qemu && make qemu-run
# (loops silently — no UART output, Ctrl-A X to quit)

# Project 2 — ZSBL + Supervisor
cd ../zsbl_supervisor && make qemu-run

# Project 3 — OpenSBI Payload
# Step 1: build OpenSBI from source (one-time)
git clone https://github.com/riscv-software-src/opensbi.git ~/eclipse-workspace/opensbi
cd ~/eclipse-workspace/opensbi
sed -i 's/firmware-genflags-y += -DFW_TEXT_START=0x0/firmware-genflags-y += -DFW_TEXT_START=0x80000000/' firmware/objects.mk
make CROSS_COMPILE=riscv64-linux-gnu- PLATFORM=generic FW_JUMP=y FW_JUMP_ADDR=0x80200000 -j$(nproc)
# Step 2: build and run payload
cd ~/eclipse-workspace/riscv-bare-metal-qemu/opensbi_payload && make qemu-run
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

**Expected output for Project 3:**
```
OpenSBI v1.8.1-38-g6d5b2b9b
   ____ ...  [banner] ...
Domain0 Next Address : 0x0000000080200000

########################################
#   OpenSBI Payload Demo               #
########################################
  Hart ID : 0x0000000000000000
  DTB     : 0x0000000082200000

[---- SBI Base Extension (0x10) ----]
  SBI Spec Version : 3.0
  Implementation   : OpenSBI
  OpenSBI Version  : 1.8
  Extensions       : TIME IPI RFENCE HSM SRST

[---- SBI Timer / sstc Extension Demo ----]
  sstc present — writing stimecmp (CSR 0x14D) directly
  Waiting for 3 timer interrupts...
[IRQ] Timer #1  time=0x000000000052DA69
[IRQ] Timer #2  time=0x00000000009F494D
[IRQ] Timer #3  time=0x0000000000EBBCEB
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
│   ├── linker.ld       # .text at 0x80000000
│   └── Makefile
├── zsbl_supervisor/
│   ├── zsbl.S              # M-mode Zero Stage Boot Loader
│   ├── supervisor_entry.S  # S-mode entry + trap vector
│   ├── supervisor.c        # S-mode C supervisor
│   ├── supervisor.h
│   ├── linker_zsbl.ld      # ZSBL at 0x80000000
│   ├── linker_super.ld     # Supervisor at 0x80200000
│   └── Makefile
├── opensbi_payload/
│   ├── entry.S         # S-mode entry at 0x80200000, stack, stvec, trap vector
│   ├── payload.c       # SBI BASE queries, stimecmp timer demo, trap handler
│   ├── payload.h       # sbiret struct, SBI extension IDs
│   ├── linker.ld       # ORIGIN = 0x80200000
│   └── Makefile        # riscv64-unknown-elf-, rv64imac_zicsr, qemu-run target
└── docs/               # GitHub Pages documentation site
    ├── index.html          # landing page + prerequisites
    ├── project1.html       # Project 1 complete guide
    ├── project2.html       # Project 2 complete guide
    ├── project3.html       # Project 3 complete guide (OpenSBI + sstc)
    └── reset-vector.html   # QEMU reset vector deep dive
```

---

## Makefile Targets

All three projects share the same targets:

| Target | Action |
|--------|--------|
| `make` / `make all` | Build ELF(s) |
| `make qemu-run` | Build and run in QEMU (no debugger) |
| `make qemu` | Build and run paused, waiting for GDB on port 1234 |
| `make clean` | Remove build artefacts |

---

## Eclipse CDT Debug

Full step-by-step Eclipse setup is in the documentation pages.  
Short version: use **GDB Hardware Debugging**, set GDB to `/usr/bin/gdb-multiarch`, host `localhost`, port `1234`.

---

## Key Concepts Covered

* RISC-V privilege levels: M-mode, S-mode
* QEMU virt reset vector (5 instructions at 0x1000)
* Bare-metal linker scripts and ELF entry points
* M-mode trap handling (`mtvec`, `mcause`, `mret`)
* Physical Memory Protection (PMP)
* Interrupt delegation (`medeleg`, `mideleg`)
* Legacy SBI (Project 2) vs SBI v1.0 (Project 3)
* SBI BASE extension — runtime feature detection
* SBI TIME extension (EID `0x54494D45`) — correct SBI v1.0 timer API
* CLINT timer and STIP injection (Project 2)
* sstc extension and `stimecmp` CSR direct write (Project 3)
* Building OpenSBI from source with `riscv64-linux-gnu-`
* S-mode trap vector (`stvec`, `sstatus`, `sret`)
* Eclipse CDT GDB Hardware Debugging

---

## Toolchain

Tested with:

* `riscv64-unknown-elf-as` / `ld` / `gcc` — GNU binutils 2.35+, GCC 10.2 (SiFive)
* `riscv64-linux-gnu-gcc` — GCC 13.x (Ubuntu 22.04) — Project 3 OpenSBI build only
* `qemu-system-riscv64` — QEMU 6.x / 8.2.2
* `gdb-multiarch` — GDB 12+
* Eclipse CDT 11+
* OpenSBI v1.8.1-38-g6d5b2b9b

## About

RISC-V bare-metal projects on QEMU virt — from reset vector to real OpenSBI.
