#!/bin/bash
set -e

echo "Compiling RISC-V source files..."
riscv64-unknown-elf-gcc -c boot1.S -o boot1.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c bm_libc.c -o bm_libc.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c beebsc.c -o beebsc.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c initframe.c -o initframe.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c qrencode.c -o qrencode.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c qrtest.c -o qrtest.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c main.c -o main.o

echo "Linking binary..."
riscv64-unknown-elf-gcc -O2 -nostdlib -Wl,--no-warn-rwx-segments -T link_uart.ld \
    boot1.o bm_libc.o beebsc.o initframe.o qrencode.o qrtest.o main.o -o qrduino_bm.elf

echo "Creating raw binary..."
riscv64-unknown-elf-objcopy -O binary qrduino_bm.elf qrduino_bm.bin

echo "Build complete! Flashing binary..."
sudo dd if=qrduino_bm.bin of=/dev/sda bs=512 seek=34 status=progress