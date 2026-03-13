CROSS   = riscv64-unknown-elf-
AS      = $(CROSS)as
CC      = $(CROSS)gcc
LD      = $(CROSS)ld

# No _sstc needed — we use raw CSR number 0x14D for stimecmp
ARCH    = -march=rv64imac_zicsr -mabi=lp64
ASFLAGS = -g $(ARCH)
CFLAGS  = -g -O0 $(ARCH) -nostdlib -nostartfiles \
          -ffreestanding -fno-builtin -Wall \
          -mcmodel=medany -mno-relax

OPENSBI = $(HOME)/eclipse-workspace/opensbi/build/platform/generic/firmware/fw_jump.elf

all: payload.elf

payload.elf: entry.o payload.o
	$(LD) -T linker.ld -nostdlib -o $@ $^

entry.o: entry.S
	$(AS) $(ASFLAGS) -o $@ $<

payload.o: payload.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.elf

qemu-run: all
	qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios $(OPENSBI) \
		-kernel payload.elf

qemu: all
	qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios $(OPENSBI) \
		-kernel payload.elf \
		-s -S

.PHONY: all clean qemu qemu-run
