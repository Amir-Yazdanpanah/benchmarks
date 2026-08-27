#!/bin/bash
riscv64-unknown-elf-gcc -march=rv64imafd -mabi=lp64d  -mcmodel=medany -ffreestanding -c boot1.S -o boot1.o
riscv64-unknown-elf-gcc -O0 -march=rv64imafd  -mabi=lp64d -mcmodel=medany -ffreestanding -c edgeDetect_bm.c -o edgeDetect_bm.o
riscv64-unknown-elf-gcc -O0 -march=rv64imafd  -mabi=lp64d -mcmodel=medany -ffreestanding -nostdlib   -T link_uart.ld boot1.o edgeDetect_bm.o -lm -lgcc -o edgeDetect_bm.elf
riscv64-unknown-elf-objcopy -O binary edgeDetect_bm.elf edgeDetect_bm.bin
sudo dd if=edgeDetect_bm.bin of=/dev/sda bs=512 seek=34 conv=fsync
