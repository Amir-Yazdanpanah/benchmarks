#!/usr/bin/env python3
"""Embed grayscale pictures in a generated C header, in command-line order."""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageOps


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("images", nargs="+", type=Path)
    parser.add_argument("--width", type=int, default=720)
    parser.add_argument("--height", type=int, default=576)
    parser.add_argument("--resize", action="store_true",
                        help="explicitly resize every picture to the requested size")
    parser.add_argument("--include-boat", action="store_true",
                        help="put existing boat.h (flat array boats) before the pictures")
    parser.add_argument("--output", type=Path, default=Path("images_batch.h"))
    args = parser.parse_args()
    if args.width < 3 or args.height < 3:
        parser.error("width and height must be at least 3")
    # Leave ample room for two output buffers, code, and stack in 16 MiB RAM.
    count = len(args.images) + int(args.include_boat)
    if (count + 4) * args.width * args.height > 14 * 1024 * 1024:
        parser.error("pictures and buffers are too large for this 16 MiB setup")
    converted = []
    for path in args.images:
        with Image.open(path) as original:
            image = ImageOps.exif_transpose(original).convert("L")
            if image.size != (args.width, args.height):
                if not args.resize:
                    parser.error(f"{path}: size {image.size}; use --resize or supply matching images")
                image = image.resize((args.width, args.height), Image.Resampling.LANCZOS)
            converted.append((path.name, image.tobytes()))
    # Write only after all inputs have been validated.
    with args.output.open("w", encoding="utf-8") as output:
        output.write("#ifndef IMAGES_BATCH_H\n#define IMAGES_BATCH_H\n#include <stdint.h>\n")
        output.write(f"#define WIDTH {args.width}\n#define HEIGHT {args.height}\n")
        output.write("struct image_entry { const uint8_t *pixels; const char *name; };\n")
        if args.include_boat:
            output.write('#include "boat.h"\n')
            output.write('_Static_assert(sizeof(boats) == WIDTH * HEIGHT, "boat.h must be a flat uint8_t array of the configured size");\n')
        for i, (_, data) in enumerate(converted):
            output.write(f"static const uint8_t picture_{i}[WIDTH * HEIGHT] = {{\n")
            for start in range(0, len(data), 24):
                output.write("    " + ",".join(str(v) for v in data[start:start+24]) + ",\n")
            output.write("};\n")
        output.write("static const struct image_entry images[] = {\n")
        if args.include_boat:
            output.write('    { boats, "boat.h" },\n')
        for i, (name, _) in enumerate(converted):
            # ASCII escaping is valid for these ordinary filename labels in C.
            output.write(f"    {{ picture_{i}, {json.dumps(name, ensure_ascii=True)} }},\n")
        output.write("};\n#define IMAGE_COUNT (sizeof(images) / sizeof(images[0]))\n#endif\n")
    print(f"Created {args.output}: {count} images, {args.width} x {args.height}, {count * args.width * args.height:,} pixel bytes")


if __name__ == "__main__":
    main()
