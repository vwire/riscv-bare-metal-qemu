CROSS   = riscv64-unknown-elf-
AS      = $(CROSS)as
LD      = $(CROSS)ld

ASFLAGS = -g -march=rv64i -mabi=lp64
LDFLAGS = -T linker.ld -nostdlib

TARGET  = main

all: $(TARGET).elf

$(TARGET).elf: main.o
	$(LD) $(LDFLAGS) -o $@ $^

main.o: main.S
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f *.o *.elf *.bin

# Run without debugger — loops forever at 'done', Ctrl-A X to quit
qemu-run: all
	qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios none \
		-kernel $(TARGET).elf

# Run paused — waiting for GDB on port 1234
qemu: all
	qemu-system-riscv64 \
		-machine virt \
		-nographic \
		-bios none \
		-kernel $(TARGET).elf \
		-s -S

.PHONY: all clean qemu qemu-run
