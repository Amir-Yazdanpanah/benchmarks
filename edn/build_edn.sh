#!/bin/bash
riscv64-unknown-elf-gcc -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c boot1.S -o boot1.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -c edn_bm.c -o edn_bm.o
riscv64-unknown-elf-gcc -O2 -march=rv64imafd -mabi=lp64d -mcmodel=medany -ffreestanding -nostdlib -T link_uart.ld boot1.o edn_bm.o -lm -lgcc -o edn_bm.elf
riscv64-unknown-elf-objcopy -O binary edn_bm.elf edn_bin.bin
sudo dd if=edn_bin.bin of=/dev/sda bs=512 seek=34 conv=fsync