#!/bin/bash
riscv64-unknown-elf-gcc -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c boot1.S -o boot1.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c slre_bm.c -o slre_bm.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -nostdlib -T link_uart.ld boot1.o slre_bm.o -lm -lgcc -o slre_bm.elf
riscv64-unknown-elf-objcopy -O binary slre_bm.elf slre_bm.bin
sudo dd if=slre_bm.bin of=/dev/sda bs=512 seek=34 conv=fsync