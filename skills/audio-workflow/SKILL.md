---
name: audio-workflow
description: Inspect, design, convert, and integrate Pocket Pixel ArduboyTones sound effects using project-local audio tooling. Use when Codex needs to list existing tones, extract a tone as JSON, draft or revise short sound effects, convert tone JSON back to PROGMEM C arrays, compare flash cost, or prepare profile-specific audio variants.
---

# Pocket Pixel Audio Workflow

Use this skill when changing or reviewing Pocket Pixel sounds.

## Tools

Open the GUI and switch to the Audio tab:

```sh
make pet-studio
```

Use Roll view for musical sketching: drag horizontally on a piano row to add a
monophonic note. Use Events view for exact `NOTE_*` names and millisecond
durations. Empty roll gaps are represented as `NOTE_REST` events.

List existing tone arrays:

```sh
tools/pet-studio/audio_tool.py list src/Assets.cpp
```

Extract all tones or one tone as JSON:

```sh
tools/pet-studio/audio_tool.py extract src/Assets.cpp --output build/pet-studio/audio.json
tools/pet-studio/audio_tool.py extract src/Assets.cpp --tone feedDone
```

Convert edited tone JSON back to a C array:

```sh
tools/pet-studio/audio_tool.py c-array assets/work/audio/new-tone.json --name newTone
```

The Makefile target exports the current tone inventory:

```sh
make audio-json
```

## Workflow

1. Start from an existing sound when possible.
2. Keep effects short; Arduboy flash budget is tight and repeated tones can
   feel harsh on the device.
3. Store unfinished audio drafts under `assets/work/audio/`.
4. Integrate accepted sounds in `src/Assets.*` as `const uint16_t PROGMEM`.
5. Trigger sounds from gameplay sparingly. Avoid restarting a tone every frame.
6. Compile stable/debug and compare size after integration.
7. Test in libretro and on hardware before accepting timing or feel.

## Profile Direction

Future profile-specific audio should be selected from static assets generated
from `profiles/*.json`, not dynamically loaded. Profile JSON may reference
named sound variants, but the build should emit fixed C++ references for the
active profile.
