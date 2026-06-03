#!/usr/bin/env python
"""Validate and render Pocket Pixel personality profiles."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


REQUIRED_TRAITS = {
    "sleepiness",
    "appetite",
    "bathroomFrequency",
    "playfulness",
    "affection",
    "independence",
    "anxiety",
    "curiosity",
    "routineSensitivity",
}

REQUIRED_BEHAVIOR = {
    "chanceSleeping",
    "chanceHungry",
    "chanceDirty",
    "chanceBored",
    "chanceAnxious",
    "chanceScratching",
    "feedCost",
    "fishPreference",
    "chickenPreference",
}

BEHAVIOR_RANGES = {
    "chanceSleeping": (1, 255),
    "chanceHungry": (1, 255),
    "chanceDirty": (1, 255),
    "chanceBored": (1, 255),
    "chanceAnxious": (1, 255),
    "chanceScratching": (1, 255),
    "feedCost": (0, 255),
    "fishPreference": (0, 100),
    "chickenPreference": (0, 100),
}

NOTE_PATTERN = re.compile(r"^(REST|[A-G](?:S|#)?[0-8])$")

DEFAULT_MENU_MELODY = [
    "E5", "E5", "F5", "G5", "G5", "F5", "E5", "D5",
    "C5", "C5", "D5", "E5", "E5", "D5", "D5",
    "E5", "E5", "F5", "G5", "G5", "F5", "E5", "D5",
    "C5", "C5", "D5", "E5", "D5", "C5", "C5",
]


def load_profile(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError("profile must be a JSON object")
    return data


def require_int(name: str, value: Any, minimum: int, maximum: int) -> None:
    if not isinstance(value, int):
        raise ValueError(f"{name} must be an integer")
    if value < minimum or value > maximum:
        raise ValueError(f"{name} must be between {minimum} and {maximum}")


def validate_profile(data: dict[str, Any]) -> None:
    if data.get("schemaVersion") != 1:
        raise ValueError("schemaVersion must be 1")
    if not isinstance(data.get("name"), str) or not data["name"].strip():
        raise ValueError("name must be a non-empty string")
    if data.get("species") != "cat":
        raise ValueError("species must currently be 'cat'")
    if not isinstance(data.get("description"), str) or not data["description"].strip():
        raise ValueError("description must be a non-empty string")

    traits = data.get("traits")
    if not isinstance(traits, dict):
        raise ValueError("traits must be an object")
    missing_traits = sorted(REQUIRED_TRAITS - set(traits))
    if missing_traits:
        raise ValueError(f"missing traits: {', '.join(missing_traits)}")
    for key in REQUIRED_TRAITS:
        require_int(f"traits.{key}", traits[key], 0, 100)

    behavior = data.get("behavior")
    if not isinstance(behavior, dict):
        raise ValueError("behavior must be an object")
    missing_behavior = sorted(REQUIRED_BEHAVIOR - set(behavior))
    if missing_behavior:
        raise ValueError(f"missing behavior values: {', '.join(missing_behavior)}")
    for key in REQUIRED_BEHAVIOR:
        minimum, maximum = BEHAVIOR_RANGES[key]
        require_int(f"behavior.{key}", behavior[key], minimum, maximum)

    assets = data.get("assets", {})
    if assets is not None and not isinstance(assets, dict):
        raise ValueError("assets must be an object when present")

    audio = data.get("audio")
    if audio is not None:
        if not isinstance(audio, dict):
            raise ValueError("audio must be an object when present")
        menu_melody = audio.get("menuMelody")
        if menu_melody is not None:
            if not isinstance(menu_melody, list) or not menu_melody:
                raise ValueError("audio.menuMelody must be a non-empty list")
            if len(menu_melody) > 64:
                raise ValueError("audio.menuMelody must contain 64 notes or fewer")
            for index, note in enumerate(menu_melody):
                if not isinstance(note, str) or not NOTE_PATTERN.match(note.upper()):
                    raise ValueError(f"audio.menuMelody[{index}] must be a note like C5, CS5, or REST")


def symbol_name(name: str) -> str:
    words = re.findall(r"[A-Za-z0-9]+", name)
    if not words:
        return "Pet"
    return "".join(word[:1].upper() + word[1:] for word in words)


def c_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=True)


def note_symbol(note: str) -> str:
    normalized = note.upper().replace("#", "S")
    if normalized == "REST":
        return "NOTE_REST"
    return f"NOTE_{normalized}"


def render_header(data: dict[str, Any]) -> str:
    validate_profile(data)
    behavior = data["behavior"]
    menu_melody = data.get("audio", {}).get("menuMelody", DEFAULT_MENU_MELODY)
    melody_values = ", ".join(note_symbol(note) for note in menu_melody)
    sym = symbol_name(data["name"])
    return f"""#pragma once

#include <Arduino.h>
#include <ArduboyTones.h>

// Generated from a Pocket Pixel personality profile. Do not edit by hand.

const uint16_t PROGMEM {sym}MenuMelody[] = {{
  {melody_values},
}};

struct PersonalityProfile {{
  const char *name;
  const uint16_t *menuMelody;
  uint8_t menuMelodyLength;
  uint8_t playfulness;
  uint8_t chanceSleeping;
  uint8_t chanceHungry;
  uint8_t chanceDirty;
  uint8_t chanceBored;
  uint8_t chanceAnxious;
  uint8_t chanceScratching;
  uint8_t feedCost;
  uint8_t fishPreference;
  uint8_t chickenPreference;
}};

constexpr PersonalityProfile {sym}Personality = {{
  {c_string(data["name"])},
  {sym}MenuMelody,
  {len(menu_melody)},
  {data["traits"]["playfulness"]},
  {behavior["chanceSleeping"]},
  {behavior["chanceHungry"]},
  {behavior["chanceDirty"]},
  {behavior["chanceBored"]},
  {behavior["chanceAnxious"]},
  {behavior["chanceScratching"]},
  {behavior["feedCost"]},
  {behavior["fishPreference"]},
  {behavior["chickenPreference"]},
}};

constexpr const PersonalityProfile &ActivePersonality = {sym}Personality;
"""


def summarize(data: dict[str, Any]) -> str:
    validate_profile(data)
    traits = data["traits"]
    behavior = data["behavior"]
    menu_melody = data.get("audio", {}).get("menuMelody", DEFAULT_MENU_MELODY)
    lines = [
        f"{data['name']} ({data['species']})",
        f"  sleepiness={traits['sleepiness']} appetite={traits['appetite']} bathroom={traits['bathroomFrequency']}",
        f"  playfulness={traits['playfulness']} affection={traits['affection']} independence={traits['independence']}",
        f"  anxiety={traits['anxiety']} curiosity={traits['curiosity']} routine={traits['routineSensitivity']}",
        "  behavior: "
        f"sleep 1/{behavior['chanceSleeping']}, "
        f"hunger 1/{behavior['chanceHungry']}, "
        f"dirty 1/{behavior['chanceDirty']}, "
        f"bored 1/{behavior['chanceBored']}, "
        f"anxious 1/{behavior['chanceAnxious']}, "
        f"scratch 1/{behavior['chanceScratching']}, "
        f"feedCost {behavior['feedCost']}, "
        f"fish {behavior['fishPreference']}, "
        f"chicken {behavior['chickenPreference']}",
        f"  audio: menuMelody {len(menu_melody)} notes",
    ]
    return "\n".join(lines)


def cmd_validate(args: argparse.Namespace) -> int:
    data = load_profile(Path(args.profile))
    validate_profile(data)
    print("profile is valid")
    return 0


def cmd_summarize(args: argparse.Namespace) -> int:
    data = load_profile(Path(args.profile))
    print(summarize(data))
    return 0


def cmd_header(args: argparse.Namespace) -> int:
    data = load_profile(Path(args.profile))
    output = render_header(data)
    if args.output:
        path = Path(args.output)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(output, encoding="utf-8")
        print(path)
    else:
        print(output, end="")
    return 0


def cmd_asset(args: argparse.Namespace) -> int:
    data = load_profile(Path(args.profile))
    validate_profile(data)
    assets = data.get("assets", {})
    value = assets.get(args.name, args.default)
    if not value:
        return 1
    print(value)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    validate = subparsers.add_parser("validate", help="validate a profile JSON")
    validate.add_argument("profile")
    validate.set_defaults(func=cmd_validate)

    summary = subparsers.add_parser("summarize", help="summarize profile values")
    summary.add_argument("profile")
    summary.set_defaults(func=cmd_summarize)

    header = subparsers.add_parser("header", help="render a C++ profile header")
    header.add_argument("profile")
    header.add_argument("--output")
    header.set_defaults(func=cmd_header)

    asset = subparsers.add_parser("asset", help="print a profile asset path")
    asset.add_argument("profile")
    asset.add_argument("name")
    asset.add_argument("--default", default="")
    asset.set_defaults(func=cmd_asset)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        return args.func(args)
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
