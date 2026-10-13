#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
if [[ ! -f images_batch.h ]]; then
    echo "Generate images_batch.h with prepare_images.py first." >&2
    exit 1
fi
prefix=${RISCV_PREFIX:-riscv64-unknown-elf-}
opt=${OPT_LEVEL:--O0}
arch=${RISCV_ARCH:-rv64imafd_zicsr}
flags=("-march=$arch" -mabi=lp64d -mcmodel=medany -ffreestanding
       -fno-builtin -fno-pie -fno-stack-protector -fno-common -msmall-data-limit=0)
mkdir -p build
"${prefix}gcc" "${flags[@]}" -c boot_batch.S -o build/boot_batch.o
"${prefix}gcc" "${flags[@]}" "$opt" -std=c11 -Wall -Wextra -c edgeDetect_batch.c -o build/edgeDetect_batch.o
"${prefix}gcc" "${flags[@]}" -nostdlib -nostartfiles -static -no-pie \
    -T link_batch.ld -Wl,-Map,build/edgeDetect_batch.map \
    build/boot_batch.o build/edgeDetect_batch.o -lgcc -o build/edgeDetect_batch.elf
"${prefix}objcopy" -O binary build/edgeDetect_batch.elf build/edgeDetect_batch.bin
"${prefix}size" build/edgeDetect_batch.elf
printf 'Built %s\n' "$PWD/build/edgeDetect_batch.bin"
echo 'Build complete. Writing the SD card is a separate command; see README.md.'
