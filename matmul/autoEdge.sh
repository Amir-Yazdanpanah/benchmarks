#!/bin/bash
set -e

riscv64-unknown-elf-gcc -march=rv64imafd -mabi=lp64d  -mcmodel=medany -ffreestanding -c boot1.S -o boot1.o
riscv64-unknown-elf-gcc -O0 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c matmul_bm.c -o matmul_bm.o
riscv64-unknown-elf-gcc -O0 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -nostdlib -T link_uart.ld boot1.o matmul_bm.o -o matmul_bm.elf
riscv64-unknown-elf-objcopy -O binary matmul_bm.elf matmul_bm.bin
sudo dd if=matmul_bm.bin of=/dev/sda bs=512 seek=34 status=progress
