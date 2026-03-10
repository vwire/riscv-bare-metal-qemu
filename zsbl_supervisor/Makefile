CROSS      = riscv64-unknown-elf-
AS         = $(CROSS)as
CC         = $(CROSS)gcc
LD         = $(CROSS)ld

ARCH       = -march=rv64imac -mabi=lp64

# NOTE: -mcmodel=medany is a GCC flag only — do NOT add to ASFLAGS
ASFLAGS    = -g $(ARCH)
CFLAGS     = -g -O0 $(ARCH) -nostdlib -nostartfiles \
             -ffreestanding -fno-builtin -Wall \
             -mcmodel=medany -mno-relax

LDFLAGS_Z  = -T linker_zsbl.ld  -nostdlib
LDFLAGS_S  = -T linker_super.ld -nostdlib

all: zsbl.elf supervisor.elf

zsbl.elf: zsbl.o
	$(LD) $(LDFLAGS_Z) -o $@ $^

zsbl.o: zsbl.S
	$(AS) $(ASFLAGS) -o $@ $<

supervisor.elf: supervisor_entry.o supervisor.o
	$(LD) $(LDFLAGS_S) -o $@ $^

supervisor_entry.o: supervisor_entry.S
	$(AS) $(ASFLAGS) -o $@ $<

supervisor.o: supervisor.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.elf *.bin

# Run without debugger
qemu-run: all
	qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios zsbl.elf \
		-device loader,file=supervisor.elf

# Run paused — waiting for GDB on port 1234
qemu: all
	qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios zsbl.elf \
		-device loader,file=supervisor.elf \
		-s -S

.PHONY: all clean qemu qemu-run
