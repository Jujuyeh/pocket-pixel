#!/usr/bin/env python3
"""Pocket Pixel Arduboy sprite conversion helpers."""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from pathlib import Path


ARRAY_RE = re.compile(
    r"const\s+uint8_t\s+PROGMEM\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\]\s*=\s*\{(.*?)\};",
    re.DOTALL,
)


def parse_number(token: str) -> int | None:
    """Parse C integer tokens and ignore symbols such as PROGMEM names."""
    token = token.strip()
    if re.fullmatch(r"0x[0-9a-fA-F]+", token):
        return int(token, 16)
    if re.fullmatch(r"\d+", token):
        return int(token, 10)
    return None


def parse_arrays(path: Path) -> list[dict]:
    """Extract Arduboy bitmap arrays with frame metadata and ASCII previews."""
    text = path.read_text(encoding="utf-8")
    assets = []
    for match in ARRAY_RE.finditer(text):
        name = match.group(1)
        values = [
            value
            for value in (parse_number(token) for token in re.split(r"[^0-9A-Fa-fx]+", match.group(2)))
            if value is not None
        ]
        if len(values) < 3:
            continue
        width, height = values[0], values[1]
        if width <= 0 or height <= 0 or width > 128 or height > 64:
            continue
        pages = math.ceil(height / 8)
        frame_bytes = width * pages
        data = values[2:]
        if len(data) < frame_bytes or len(data) % frame_bytes != 0:
            continue
        frame_count = len(data) // frame_bytes
        assets.append(
            {
                "name": name,
                "width": width,
                "height": height,
                "data": data,
                "frameBytes": frame_bytes,
                "frameCount": frame_count,
                "bytes": 2 + len(data),
                "ascii": bitmap_to_ascii(width, height, data[:frame_bytes]),
                "framesAscii": [
                    bitmap_to_ascii(width, height, data[start : start + frame_bytes])
                    for start in range(0, len(data), frame_bytes)
                ],
            }
        )
    return assets


def bitmap_to_ascii(width: int, height: int, data: list[int]) -> str:
    """Convert Arduboy page bytes into #/. rows for review and diffs."""
    pages = math.ceil(height / 8)
    rows = []
    for y in range(height):
        row = []
        for x in range(width):
            byte = data[(y // 8) * width + x]
            bit = (byte >> (y % 8)) & 1
            row.append("#" if bit == 0 else ".")
        rows.append("".join(row))
    return "\n".join(rows)


def ascii_to_bitmap(text: str) -> tuple[int, int, list[int]]:
    """Convert #/. art into Arduboy page bytes."""
    rows = [
        line.strip()
        for line in text.splitlines()
        if line.strip() and not line.strip().startswith("//")
    ]
    if not rows:
        raise SystemExit("ASCII input is empty")
    width = max(len(row) for row in rows)
    height = len(rows)
    pages = math.ceil(height / 8)
    data = []
    for page in range(pages):
        for x in range(width):
            byte = 0xFF
            for bit in range(8):
                y = page * 8 + bit
                if y < height and x < len(rows[y]) and rows[y][x] == "#":
                    byte &= ~(1 << bit)
            data.append(byte)
    return width, height, data


def c_array(name: str, width: int, height: int, data: list[int]) -> str:
    """Serialize one bitmap or same-sized frame set as C source bytes."""
    lines = [f"const uint8_t PROGMEM {name}[] = {{", f"{width}, {height},"]
    for start in range(0, len(data), width):
        chunk = ", ".join(f"0x{value:02x}" for value in data[start : start + width])
        lines.append(f"{chunk},")
    lines.append("};")
    return "\n".join(lines)


def find_asset(assets: list[dict], name: str) -> dict:
    """Return one named asset or include candidates in the error."""
    for asset in assets:
        if asset["name"] == name:
            return asset
    available = ", ".join(asset["name"] for asset in assets)
    raise SystemExit(f"Asset '{name}' not found. Available: {available}")


def command_extract(args: argparse.Namespace) -> None:
    """Emit a complete JSON asset index for Pet Studio."""
    assets = parse_arrays(args.source)
    payload = {
        "source": str(args.source),
        "count": len(assets),
        "assets": assets,
    }
    text = json.dumps(payload, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text + "\n", encoding="utf-8")
    else:
        print(text)


def command_list(args: argparse.Namespace) -> None:
    """Print dimensions, frame count, and storage cost for each asset."""
    for asset in parse_arrays(args.source):
        print(
            f"{asset['name']}\t{asset['width']}x{asset['height']}"
            f"\t{asset['frameCount']} frame(s)\t{asset['bytes']} bytes"
        )


def command_ascii(args: argparse.Namespace) -> None:
    """Print a single frame as #/. art for quick terminal inspection."""
    asset = find_asset(parse_arrays(args.source), args.asset)
    frame = args.frame
    if frame < 0 or frame >= asset["frameCount"]:
        raise SystemExit(f"Frame must be between 0 and {asset['frameCount'] - 1}")
    print(asset["framesAscii"][frame])


def command_from_ascii(args: argparse.Namespace) -> None:
    """Turn a saved ASCII draft into a C bitmap array."""
    width, height, data = ascii_to_bitmap(args.input.read_text(encoding="utf-8"))
    print(c_array(args.name, width, height, data))


def command_png_to_c(args: argparse.Namespace) -> None:
    """Threshold a PNG into the one-bit Arduboy bitmap format."""
    try:
        from PIL import Image
    except ImportError as exc:
        raise SystemExit("Pillow is required for PNG import. Enter nix develop.") from exc

    image = Image.open(args.input).convert("RGBA")
    width, height = image.size
    pages = math.ceil(height / 8)
    data = []
    pixels = image.load()
    for page in range(pages):
        for x in range(width):
            byte = 0xFF
            for bit in range(8):
                y = page * 8 + bit
                if y >= height:
                    continue
                r, g, b, a = pixels[x, y]
                luma = r * 0.299 + g * 0.587 + b * 0.114
                if a >= 128 and luma < args.threshold:
                    byte &= ~(1 << bit)
            data.append(byte)
    print(c_array(args.name, width, height, data))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(required=True)

    extract = subparsers.add_parser("extract", help="extract Arduboy bitmap arrays as JSON")
    extract.add_argument("source", type=Path)
    extract.add_argument("--output", type=Path)
    extract.set_defaults(func=command_extract)

    list_cmd = subparsers.add_parser("list", help="list bitmap arrays")
    list_cmd.add_argument("source", type=Path)
    list_cmd.set_defaults(func=command_list)

    ascii_cmd = subparsers.add_parser("ascii", help="print one bitmap as #/. ASCII")
    ascii_cmd.add_argument("source", type=Path)
    ascii_cmd.add_argument("--asset", required=True)
    ascii_cmd.add_argument("--frame", type=int, default=0)
    ascii_cmd.set_defaults(func=command_ascii)

    from_ascii = subparsers.add_parser("from-ascii", help="convert a #/. ASCII grid to a C array")
    from_ascii.add_argument("input", type=Path)
    from_ascii.add_argument("--name", required=True)
    from_ascii.set_defaults(func=command_from_ascii)

    png_to_c = subparsers.add_parser("png-to-c", help="convert a PNG to a C array")
    png_to_c.add_argument("input", type=Path)
    png_to_c.add_argument("--name", required=True)
    png_to_c.add_argument("--threshold", type=int, default=128)
    png_to_c.set_defaults(func=command_png_to_c)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
