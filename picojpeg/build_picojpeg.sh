#!/bin/bash
set -e

riscv64-unknown-elf-gcc -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c boot1.S -o boot1.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c picojpeg.c -o picojpeg.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c picojpeg_bm.c -o picojpeg_bm.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -nostdlib -T link_uart.ld boot1.o picojpeg.o picojpeg_bm.o -lm -lgcc -o picojpeg_bm.elf
riscv64-unknown-elf-objcopy -O binary picojpeg_bm.elf picojpeg_bm.bin

echo "[SUCCESS] Build complete. Writing binary payload..."
sudo dd if=picojpeg_bm.bin of=/dev/sda bs=512 seek=34 conv=fsync