#!/usr/bin/env python3
"""Insert Pocket Pixel as a new game entry in a decompiled FX-C flashcart."""

from __future__ import annotations

import argparse
import csv
import shutil
from pathlib import Path


HEADER = ["List", "Discription", "Title screen", "Hex file", "Data file", "Save file"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flashcart-dir", required=True, type=Path)
    parser.add_argument("--category", required=True, type=int)
    parser.add_argument("--title", default="Pocket Pixel")
    parser.add_argument("--replace-title")
    parser.add_argument("--hex", required=True, type=Path)
    parser.add_argument("--banner", required=True, type=Path)
    return parser.parse_args()


def read_rows(index: Path) -> list[list[str]]:
    with index.open(newline="", encoding="utf-8") as handle:
        reader = csv.reader(handle, delimiter=";")
        rows = list(reader)
    if not rows or rows[0][:6] != HEADER:
        raise SystemExit(f"Unexpected flashcart index header: {index}")
    return rows


def next_slot(rows: list[list[str]], category: int) -> int:
    slots = []
    for row in rows[1:]:
        if not row or row[0] != str(category):
            continue
        title_path = row[2]
        try:
            slots.append(int(Path(title_path).parent.name))
        except ValueError:
            continue
    if not slots:
        raise SystemExit(f"Category {category} does not exist in the flashcart index.")
    return max(slots) + 1


def remove_existing_title(rows: list[list[str]], title: str) -> list[list[str]]:
    return [rows[0]] + [row for row in rows[1:] if len(row) < 2 or row[1] != title]


def main() -> None:
    args = parse_args()
    index = args.flashcart_dir / "flashcart-index.csv"
    if not index.is_file():
        raise SystemExit(f"Missing flashcart index: {index}")
    if not args.hex.is_file():
        raise SystemExit(f"Missing HEX: {args.hex}")
    if not args.banner.is_file():
        raise SystemExit(f"Missing banner: {args.banner}")

    rows = read_rows(index)
    replace_title = args.replace_title or args.title
    rows = remove_existing_title(rows, replace_title)
    category = str(args.category)
    slot = next_slot(rows, args.category)
    target_dir = args.flashcart_dir / category / str(slot)
    target_dir.mkdir(parents=True, exist_ok=False)
    title_screen = f"{category}/{slot}/{slot}.png"
    hex_file = f"{category}/{slot}/{slot}.hex"
    shutil.copyfile(args.banner, args.flashcart_dir / title_screen)
    shutil.copyfile(args.hex, args.flashcart_dir / hex_file)
    rows.append([category, args.title, title_screen, hex_file, "", ""])

    with index.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle, delimiter=";", lineterminator="\n")
        writer.writerows(rows)

    print(f"Inserted {args.title} at {category}/{slot}/{slot}")


if __name__ == "__main__":
    main()
