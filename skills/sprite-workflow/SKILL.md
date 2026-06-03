---
name: sprite-workflow
description: Inspect, edit, slice, convert, and optimize Pocket Pixel Arduboy 1-bit sprites using the project-local Pet Studio GUI and CLI helpers. Use when Codex needs to work with bitmap assets, pixel art handoff, sprite slicing, PNG or ASCII sprite conversion, PROGMEM bitmap storage, or flash/RAM sprite tradeoffs.
---

# Pocket Pixel Sprite Workflow

Use this skill when changing or reviewing sprites for Pocket Pixel.

## Tools

Open the GUI:

```sh
make pet-studio
```

The legacy alias still works:

```sh
make sprite-studio
```

Use the CLI helper:

```sh
tools/pet-studio/sprite_tool.py --help
```

The GUI is a local Pet Studio under `tools/pet-studio/`. It has profile,
sprite, and audio surfaces. The sprite mode can paste/import Arduboy C arrays,
ASCII grids, or PNG files; edit pixels; slice frame strips; and export C arrays,
ASCII grids, or PNG previews. When launched with `make pet-studio`, it runs
through the project-local server and can save edited sprite drafts under
`assets/work/sprites/`.

The GUI lists both integrated sprites from `src/Assets.cpp` and saved drafts
from `assets/work/sprites/*.h`. Use drafts for collaborative artwork that is not
ready to compile into the game.

Pet Studio handles Arduboy multi-frame arrays and numbered single-frame
siblings. For example, `food` loads as one multi-frame array, while
`PET_IDLE1`/`PET_IDLE2` load together as frames because they share a prefix and
dimensions.

For pixel editing, the GUI supports draw, erase, toggle, line, brush size,
continuous brush strokes, and hover coordinates. Right-click erases temporarily
without changing the selected tool.

When editing a shared base design across frames, use `Copy frame` and
`Paste frame`. Use `Paste black` when the copied frame should only add black
pixels to the current frame instead of replacing white/background pixels.

The CLI is for repeatable agent work and review:

```sh
tools/pet-studio/sprite_tool.py list src/Assets.cpp
tools/pet-studio/sprite_tool.py ascii src/Assets.cpp --asset poo1
tools/pet-studio/sprite_tool.py extract src/Assets.cpp --output build/pet-studio/assets.json
tools/pet-studio/sprite_tool.py png-to-c assets/work/new-sprite.png --name newSprite
tools/pet-studio/sprite_tool.py from-ascii assets/work/new-sprite.txt --name newSprite
```

## Workflow

1. Inspect the current sprite and its storage cost:

```sh
tools/pet-studio/sprite_tool.py list src/Assets.cpp
nix develop -c make size
```

2. Convert the sprite to ASCII or open it in Pet Studio for visual edits.
3. Keep black pixels as `#`, white/background pixels as `.`.
4. Export Arduboy arrays with width and height as the first two bytes.
5. Store persistent bitmap assets in `src/Assets.*` and keep them in `PROGMEM`.
6. Prefer procedural drawing only when it is clearly cheaper or easier to tune.
7. Compile and compare size before committing:

```sh
nix develop -c make size
nix develop -c make size-debug
nix develop -c make compile
nix develop -c make compile-debug
```

8. Update `docs/sprite-guidelines.md` when a new sprite convention, failed
   optimization, or useful handoff rule is discovered.
9. If Pet Studio itself gains a workflow feature, update this skill so future
   agents know how to use it.

## Decision Rules

- Use exact native dimensions; do not scale sprites in code unless the game
  already does so for that asset.
- Keep animation frame dimensions consistent unless the caller is designed for
  per-frame sizes.
- For editable mask drafts in this project, black means visible/opaque and
  white means transparent unless the caller documents a different convention.
- Check very small sprites empirically. A clever delta can cost more flash than
  duplicated bitmap bytes once patching code is included.
- Use ASCII grids for quick collaboration when the user wants to describe or
  adjust a tiny asset by hand.
- Use PNG import for user-drawn art, title cards, or frame strips.
- Treat Pet Studio output as a starting point. Always run the game in
  libretro or on hardware before relying on one-pixel details.

## User Handoff

Ask for one of these when visual judgement matters:

- exact-scale 1-bit PNG,
- enlarged PNG with stated native size,
- ASCII grid,
- rough sketch plus target width/height and animation notes.

When giving the user art homework, be specific about size, black/white rules,
frame order, and which pixels are active hitboxes or brush areas.
