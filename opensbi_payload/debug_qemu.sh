#!/bin/bash
qemu-system-riscv64 \
    -machine virt \
    -nographic \
    -bios ~/eclipse-workspace/opensbi/build/platform/generic/firmware/fw_jump.elf \
    -kernel ~/eclipse-workspace/opensbi/opensbi_payload/payload.elf \
    -S -s
