#!/usr/bin/env python3
"""Port-aware Arduboy FX/FX-C flashcart backup and writer helper."""

from __future__ import annotations

import argparse
import time
from pathlib import Path

from serial import Serial
from serial.tools.list_ports import comports


COMPATIBLE_DEVICES = [
    "VID:PID=2341:0036", "VID:PID=2341:8036",
    "VID:PID=2A03:0036", "VID:PID=2A03:8036",
    "VID:PID=2341:0037", "VID:PID=2341:8037",
    "VID:PID=2A03:0037", "VID:PID=2A03:8037",
    "VID:PID=2341:0237", "VID:PID=2341:8237",
    "VID:PID=1B4F:9205", "VID:PID=1B4F:9206",
    "VID:PID=239A:000E", "VID:PID=239A:800E",
]

MANUFACTURERS = {
    0x01: "Spansion",
    0x14: "Cypress",
    0x1C: "EON",
    0x1F: "Adesto(Atmel)",
    0x20: "Micron",
    0x37: "AMIC",
    0x9D: "ISSI",
    0xC2: "General Plus",
    0xC8: "Giga Device",
    0xBF: "Microchip",
    0xEF: "Winbond",
}

PAGESIZE = 256
BLOCKSIZE = 65536


def arduboy_ports() -> list:
    ports = []
    for port in comports():
        match_index = next((i for i, vidpid in enumerate(COMPATIBLE_DEVICES) if vidpid in port.hwid), None)
        if match_index is not None:
            ports.append((port, (match_index & 1) == 0))
    return ports


def list_ports() -> None:
    ports = arduboy_ports()
    if not ports:
        print("No Arduboy-compatible serial ports detected.")
        return
    for port, bootloader_active in ports:
        mode = "bootloader" if bootloader_active else "sketch"
        print(f"{port.device}\t{mode}\t{port.description}\t{port.hwid}")


def port_is_bootloader(device: str) -> bool:
    for port, bootloader_active in arduboy_ports():
        if port.device == device:
            return bootloader_active
    raise SystemExit(f"Port is not an Arduboy-compatible device: {device}")


def wait_for_port(device: str, timeout: float = 8.0) -> None:
    start = time.time()
    while time.time() - start < timeout:
        if any(port.device == device for port, _ in arduboy_ports()):
            return
        time.sleep(0.1)
    raise SystemExit(f"Timed out waiting for {device}")


def open_bootloader(device: str) -> Serial:
    if not port_is_bootloader(device):
        print(f"Selecting bootloader mode on {device}...")
        trigger = Serial(device, 1200)
        trigger.close()
        time.sleep(0.5)
        wait_for_port(device)

    print(f"Opening {device} ...")
    last_error = None
    for _ in range(20):
        try:
            return Serial(device, 57600, timeout=2)
        except Exception as exc:  # pragma: no cover - hardware path
            last_error = exc
            time.sleep(0.25)
    raise SystemExit(f"Failed to open {device}: {last_error}")


def read_exact(serial: Serial, length: int) -> bytes:
    data = serial.read(length)
    if len(data) != length:
        raise SystemExit(f"Expected {length} bytes, got {len(data)}")
    return data


def get_version(serial: Serial) -> int:
    serial.write(b"V")
    return int(read_exact(serial, 2))


def get_jedec_id(serial: Serial) -> bytearray:
    serial.write(b"j")
    jedec_id = read_exact(serial, 3)
    time.sleep(0.5)
    serial.write(b"j")
    if read_exact(serial, 3) != jedec_id or jedec_id in (b"\x00\x00\x00", b"\xff\xff\xff"):
        raise SystemExit("No flash cart detected.")
    return bytearray(jedec_id)


def print_flash_info(jedec_id: bytearray) -> tuple[int, int]:
    manufacturer = MANUFACTURERS.get(jedec_id[0], "unknown")
    capacity = 1 << ((jedec_id[2] >> 4) * 10 + (jedec_id[2] & 0x0F) + 6)
    carts = (capacity + 16777215) // 16777216
    print(f"Flash cart JEDEC ID    : {jedec_id[0]:02X}{jedec_id[1]:02X}{jedec_id[2]:02X}")
    print(f"Flash cart Manufacturer: {manufacturer}")
    print(f"Flash cart capacity    : {capacity // 1024} Kbyte")
    return capacity, carts


def select_cart(serial: Serial, cart: int) -> None:
    serial.write(b"T")
    serial.write(bytearray([cart]))
    read_exact(serial, 1)


def set_address(serial: Serial, page: int) -> None:
    serial.write(b"A")
    serial.write(bytearray([page >> 8, page & 0xFF]))
    read_exact(serial, 1)


def backup(port: str, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    serial = open_bootloader(port)
    try:
        if get_version(serial) < 13:
            raise SystemExit("Bootloader has no flash cart support.")
        capacity, carts = print_flash_info(get_jedec_id(serial))
        if carts != 1:
            raise SystemExit("Multi-cart backup is not supported by this project helper yet.")
        blocks = capacity // BLOCKSIZE
        print(f"Writing flash image to {output}")
        with output.open("wb") as handle:
            for block in range(blocks):
                serial.write(b"x\xC1" if block & 1 else b"x\xC0")
                read_exact(serial, 1)
                print(f"\rReading block {block + 1}/{blocks}", end="")
                set_address(serial, block * BLOCKSIZE // PAGESIZE)
                serial.write(b"g")
                serial.write(bytearray([(BLOCKSIZE >> 8) & 0xFF, BLOCKSIZE & 0xFF]))
                serial.write(b"C")
                handle.write(read_exact(serial, BLOCKSIZE))
        print("\nBackup complete.")
    finally:
        serial.write(b"x\x44")
        read_exact(serial, 1)
        serial.write(b"E")
        serial.read(1)
        serial.close()


def write_image(port: str, image: Path, verify: bool) -> None:
    data = image.read_bytes()
    if len(data) % BLOCKSIZE:
        data += b"\xff" * (BLOCKSIZE - len(data) % BLOCKSIZE)
    serial = open_bootloader(port)
    try:
        if get_version(serial) < 13:
            raise SystemExit("Bootloader has no flash cart support.")
        print_flash_info(get_jedec_id(serial))
        blocks = len(data) // BLOCKSIZE
        print(f"Writing {image} to {port}")
        for block in range(blocks):
            serial.write(b"x\xC2")
            read_exact(serial, 1)
            print(f"\rWriting block {block + 1}/{blocks}", end="")
            page = block * BLOCKSIZE // PAGESIZE
            set_address(serial, page)
            serial.write(bytearray([ord("B"), (BLOCKSIZE >> 8) & 0xFF, BLOCKSIZE & 0xFF, ord("C")]))
            serial.write(data[block * BLOCKSIZE:(block + 1) * BLOCKSIZE])
            read_exact(serial, 1)
            if verify:
                serial.write(b"x\xC1")
                read_exact(serial, 1)
                set_address(serial, page)
                serial.write(bytearray([ord("g"), (BLOCKSIZE >> 8) & 0xFF, BLOCKSIZE & 0xFF, ord("C")]))
                if read_exact(serial, BLOCKSIZE) != data[block * BLOCKSIZE:(block + 1) * BLOCKSIZE]:
                    raise SystemExit(f"Verify failed on block {block + 1}.")
        print("\nWrite complete.")
    finally:
        serial.write(b"x\x44")
        read_exact(serial, 1)
        serial.write(b"E")
        serial.read(1)
        serial.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("list")
    backup_parser = subparsers.add_parser("backup")
    backup_parser.add_argument("--port", required=True)
    backup_parser.add_argument("--output", required=True, type=Path)
    write_parser = subparsers.add_parser("write")
    write_parser.add_argument("--port", required=True)
    write_parser.add_argument("--image", required=True, type=Path)
    write_parser.add_argument("--no-verify", action="store_true")
    args = parser.parse_args()

    if args.command == "list":
        list_ports()
    elif args.command == "backup":
        backup(args.port, args.output)
    elif args.command == "write":
        write_image(args.port, args.image, not args.no_verify)


if __name__ == "__main__":
    main()
