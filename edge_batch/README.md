# Six pictures in one bare-metal Rocket program

This batch version uses your UART address (0x64000000), GPIO address
(0x64002000), RV64IMAFD/LP64D target, 16 MiB RAM region at 0x80000000,
and existing sector-34 SD boot method. It processes all pictures in order,
once per reset. It does not need Linux, a filesystem driver, or six programs.
The pictures become byte arrays inside the single executable.

## 1. Prepare five new pictures

Extract this folder on the computer/container with your RISC-V toolchain.
Put your existing boat.h beside these files. It was not attached, so is not
included here. Its `boats` array must be flat uint8_t data with 720*576 bytes.
Put the five new JPG/PNG pictures in a pictures/ subdirectory.

Install the image conversion dependency if needed:

```bash
python3 -m pip install Pillow
```

Convert the five new pictures and include the original boat as picture 1:

```bash
python3 prepare_images.py --include-boat --width 720 --height 576 --resize \
    pictures/image2.jpg pictures/image3.jpg pictures/image4.jpg \
    pictures/image5.jpg pictures/image6.jpg
```

Replace those names with your real filenames. Images are converted to 8-bit
grayscale; --resize resizes them to 720x576 (possibly changing aspect ratio).
For already matching sizes, omit --resize to reject dimension mismatches.
Six pictures consume 2,488,320 pixel bytes, about 2.37 MiB. Output buffers
consume another 1,658,880 bytes. The linker checks space including the stack.

If you have the original boat as a JPG/PNG instead of boat.h, omit
--include-boat and supply all six image paths. To run only five new pictures,
omit --include-boat and supply just those five paths.

## 2. Compile once

```bash
bash build_batch.sh
```

The script produces build/edgeDetect_batch.bin and never writes a disk.
It keeps -O0, as in your script. Modern tools require the explicit Zicsr suffix
for the CSR instructions already used by your original startup. For an older
toolchain that does not recognize this suffix, use:

```bash
RISCV_ARCH=rv64imafd bash build_batch.sh
```

If the installed toolchain has another name, set RISCV_PREFIX.

## 3. Write the single binary once

Insert the same, already working SD card in the PC. Identify it:

```bash
lsblk -o NAME,SIZE,MODEL,TYPE,MOUNTPOINTS
```

Unmount any mounted SD partitions individually, for example:

```bash
sudo umount /dev/sdX1
```

Replace sdX with the SD card device. Write to the whole card with seek=34,
as your current boot flow does. Do not also use seek=34 on a partition path.

```bash
sudo dd if=build/edgeDetect_batch.bin of=/dev/sdX bs=512 seek=34 conv=fsync status=progress
sync
```

This overwrites the raw boot-payload area. Select the SD card, not your system
disk. Keep the existing partition/boot layout. The payload area must be large
enough for the binary; do not overwrite a following partition. Do not format
the card or copy JPG files onto its filesystem for this approach.

## 4. Run the batch

Move the SD card to VC707, keep the working FPGA bitstream, open your usual
UART at 115200 baud, and press SW8. Expect:

```text
BATCH_BEGIN images=6
IMAGE_START 1 boat.h
IMAGE_DONE 1 cycles=...
IMAGE_START 2 image2.jpg
IMAGE_DONE 2 cycles=...
...
IMAGE_START 6 image6.jpg
IMAGE_DONE 6 cycles=...
BATCH_DONE
```

Each image creates one HIGH pulse on your existing GPIO pin 0. Configure
the CX3300 acquisition to retain all six pulses (or use a suitably long
capture). No CX3300 control/remote commands are implemented here.
Pressing SW8 again repeats the whole batch.

## Measurement behavior and fixes

The GPIO HIGH interval includes Sobel plus RGB-overlay generation, matching
the two computations in your original main. Cycle counts cover that work
and the final memory fence; the physical pulse also includes marker overhead.
UART messages are outside the HIGH interval. A low interval of 5,000,000 CPU
cycles precedes each image; its time in seconds depends on your Rocket clock.
No result images are sent over UART, matching your currently commented-out
export calls. `edges` and `overlay` remain in RAM; the program then spins.

Your original overlay reads `gray[]` even though filling that array is
commented out. This version reads the current picture instead. It reuses
the same edge and overlay buffers for all images, and initializes BSS once
so the never-written edge borders are zero. Startup places _start at the
beginning of the raw binary and defines the stack symbol only in the linker.

This batch does NOT reset caches or predictors between pictures. Each run
inherits processor state from the previous run. Cold-cache measurements need
a separate, supported cache-reset strategy for your generated Rocket; a gap
or fence.i is not a general data-cache flush. Preserve the same optimization
and measurement policy when comparing pictures.

## Verification performed

The converter, dimension checks, ordering, existing-boat path, C syntax,
shell syntax, and Sobel/overlay computation were checked on the host.
The RISC-V toolchain and FPGA were unavailable in the preparation environment,
so target compilation, SD boot, and GPIO measurements require verification
on your setup.
