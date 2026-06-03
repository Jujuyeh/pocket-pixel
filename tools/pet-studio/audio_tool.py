#!/usr/bin/env python3
"""Pocket Pixel ArduboyTones inspection and conversion helpers."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


TONE_RE = re.compile(
    r"const\s+uint16_t\s+PROGMEM\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[\]\s*=\s*\{(.*?)\};",
    re.DOTALL,
)


def strip_comments(text: str) -> str:
    """Remove C/C++ comments before tokenizing ArduboyTones arrays."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//.*", "", text)


def parse_token(token: str) -> str | int:
    """Keep symbolic notes/delimiters as strings and convert numeric durations."""
    token = token.strip()
    if re.fullmatch(r"0x[0-9a-fA-F]+", token):
        return int(token, 16)
    if re.fullmatch(r"\d+", token):
        return int(token, 10)
    return token


def parse_tones(path: Path) -> list[dict[str, Any]]:
    """Extract PROGMEM tone arrays into an editor-friendly JSON shape."""
    text = strip_comments(path.read_text(encoding="utf-8"))
    tones = []
    for match in TONE_RE.finditer(text):
        name = match.group(1)
        tokens = [
            parse_token(token)
            for token in match.group(2).split(",")
            if token.strip()
        ]
        if not tokens or tokens[-1] != "TONES_END":
            continue
        events = []
        body = tokens[:-1]
        for index in range(0, len(body), 2):
            if index + 1 >= len(body):
                break
            events.append({"note": body[index], "duration": body[index + 1]})
        tones.append(
            {
                "name": name,
                "events": events,
                "tokens": tokens,
                "eventCount": len(events),
                "bytes": len(tokens) * 2,
            }
        )
    return tones


def find_tone(tones: list[dict[str, Any]], name: str) -> dict[str, Any]:
    """Return one named tone or fail with the available choices."""
    for tone in tones:
        if tone["name"] == name:
            return tone
    available = ", ".join(tone["name"] for tone in tones)
    raise SystemExit(f"Tone '{name}' not found. Available: {available}")


def c_token(value: str | int) -> str:
    """Format one event token back into C source."""
    return str(value)


def c_array(name: str, events: list[dict[str, Any]]) -> str:
    """Serialize editor events as an ArduboyTones PROGMEM array."""
    lines = [f"const uint16_t PROGMEM {name}[] = {{"]
    for event in events:
        lines.append(f"{c_token(event['note'])}, {c_token(event['duration'])},")
    lines.append("TONES_END };")
    return "\n".join(lines)


def command_list(args: argparse.Namespace) -> None:
    """Print a compact inventory for humans and scripts."""
    for tone in parse_tones(args.source):
        print(f"{tone['name']}\t{tone['eventCount']} event(s)\t{tone['bytes']} bytes")


def command_extract(args: argparse.Namespace) -> None:
    """Dump one tone or the full tone index as JSON."""
    tones = parse_tones(args.source)
    if args.tone:
        payload: dict[str, Any] = find_tone(tones, args.tone)
    else:
        payload = {
            "source": str(args.source),
            "count": len(tones),
            "tones": tones,
        }
    text = json.dumps(payload, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text + "\n", encoding="utf-8")
    else:
        print(text)


def command_c_array(args: argparse.Namespace) -> None:
    """Convert a saved JSON draft back into copyable C source."""
    data = json.loads(args.input.read_text(encoding="utf-8"))
    events = data.get("events")
    if not isinstance(events, list):
        raise SystemExit("JSON input must contain an events array")
    print(c_array(args.name, events))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(required=True)

    list_cmd = subparsers.add_parser("list", help="list ArduboyTones arrays")
    list_cmd.add_argument("source", type=Path)
    list_cmd.set_defaults(func=command_list)

    extract = subparsers.add_parser("extract", help="extract tones as JSON")
    extract.add_argument("source", type=Path)
    extract.add_argument("--tone")
    extract.add_argument("--output", type=Path)
    extract.set_defaults(func=command_extract)

    carray = subparsers.add_parser("c-array", help="convert tone JSON to a C array")
    carray.add_argument("input", type=Path)
    carray.add_argument("--name", required=True)
    carray.set_defaults(func=command_c_array)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
