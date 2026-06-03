# Sprite Guidelines

Pocket Pixel targets a 128x64 1-bit Arduboy display. Sprites should stay small,
explicit, and easy to verify in emulator and on hardware.

## When To Use Bitmaps

Use `PROGMEM` bitmap sprites when the shape is organic, detailed, reused often,
or hard to express with simple drawing primitives.

Good bitmap candidates:

- pet body poses,
- expressive faces,
- bowls, food, toys, and item art,
- title/catalog art,
- minigame objects with character.

Keep bitmap arrays in `src/Assets.*`. Prefer one canonical sprite plus small
overlays or deltas when animation frames mostly share the same pixels.

For very small sprites, duplicated bitmap frames can still be cheaper than
procedural deltas. The `poo1`/`poo2` animation is intentionally kept as two
bitmaps. We tested both pixel-by-pixel stink deltas and a split base-poop plus
tiny stink sprites; both used more flash than storing the second 16x16 frame.

The same rule currently applies to `PET_IDLE1`/`PET_IDLE2`: a compact
page-aligned delta representation was tested, but the patching code cost more
flash than keeping both 32x32 bitmap frames.

`PET_IDLE1`/`PET_IDLE2` are cropped to a shared 26x24 bounding box. Their draw
position is offset by the removed 2 left pixels and 5 top pixels so the pet
stays in the same screen position. Keep future idle frames on the same 26x24
canvas unless the draw constants in `src/Game.cpp` are updated deliberately.

## When To Draw Procedurally

Use drawing code when the sprite is geometric, mostly empty, or made from simple
lines and dots. This avoids storing full frames of white background.

Good procedural candidates:

- sleep `Z` animation,
- clean tools,
- UI icons,
- simple coins,
- cursor/brush previews,
- debug-only markers.

The sleep `Z` animation is the current example: five bitmap frames were replaced
with three small procedural glyphs combined by frame.

## Pixel Art Handoff

The most useful art input is a tiny 1-bit reference with clear dimensions.

Good formats to hand off:

- a PNG at exact Arduboy scale, using only black and white,
- an enlarged mockup with a stated native size, for example "drawn at 8x zoom,
  native sprite is 16x16",
- an ASCII grid using `#` for black and `.` for white,
- a frame strip with one consistent frame size.

For each sprite, specify:

- native width and height,
- transparent/white background expectations,
- animation frame order,
- whether the sprite should overwrite, erase, or draw as an overlay,
- where its active hitbox or brush point should be.

For animation frames with different dimensions, define the shared registration
point explicitly. `Play` cat frames use a shared bottom-left anchor so the feet
stay aligned with the background floor even when each frame has a different
width or height.

Editable mask drafts use black pixels for visible/opaque and white pixels for
transparent. The Play renderer interprets `playCatPawMask_*` and
`playHandMask` with that Sprite-Studio-friendly convention instead of Arduboy's
external-mask bit sense.

## Pet Studio

Pocket Pixel includes a local profile-aware asset tool under
`tools/pet-studio/`. It has Preferences, Sprites, and Audio modes.

Open the GUI:

```sh
make pet-studio
```

The old sprite-focused alias also works:

```sh
make sprite-studio
```

This target starts the project-local Pet Studio server at
`http://127.0.0.1:8123/tools/pet-studio/`. The GUI can load profiles from
`profiles/`, sprites from `src/Assets.cpp`, sounds from `src/Assets.cpp`, and
saved drafts under `assets/work/`.

The project sprite selector includes integrated sprites and saved drafts from
`assets/work/sprites/*.h`. Keep unfinished collaboration art as drafts until it
has been reviewed in libretro and intentionally integrated into `src/Assets.*`.

The Sprites mode also has a profile-banner workflow. `Load banner` imports the
active profile's `assets.banner` PNG as a 128x64 editable canvas. `Save banner`
writes the current 128x64 canvas back as the profile's FX catalog banner PNG.
Use this path for banner art instead of editing `assets/fx/*.png` outside the
repo tooling.

Pet Studio supports both Arduboy multi-frame arrays, such as `food`, and
legacy numbered single-frame arrays, such as `PET_IDLE1`/`PET_IDLE2`. When a
numbered sprite is loaded, matching siblings with the same prefix and dimensions
are treated as frames for editing.

Current editing features:

- draw, erase, toggle, and straight-line tools,
- configurable square brush size,
- continuous strokes that fill gaps between pointer events,
- right-click temporary erase,
- hover coordinates for exact pixel placement,
- frame copy/paste and black-only paste for multi-frame edits.

Use the CLI for repeatable inspection and conversion:

```sh
tools/pet-studio/sprite_tool.py list src/Assets.cpp
tools/pet-studio/sprite_tool.py ascii src/Assets.cpp --asset poo1
tools/pet-studio/sprite_tool.py extract src/Assets.cpp --output build/pet-studio/assets.json
tools/pet-studio/sprite_tool.py png-to-c assets/work/new-sprite.png --name newSprite
tools/pet-studio/sprite_tool.py from-ascii assets/work/new-sprite.txt --name newSprite
```

Profiles can be selected or created from the compact top bar. The Preferences
mode has a structured Form view for profile sections and a Raw view for direct
JSON editing; saving synchronizes both views. Audio can be edited from the Audio
tab or handled by the CLI. The GUI has a horizontally expanded piano-roll view
for drawing monophonic ArduboyTones sequences and an event-list view for exact
`NOTE_*` and duration edits:

```sh
tools/pet-studio/audio_tool.py list src/Assets.cpp
tools/pet-studio/audio_tool.py extract src/Assets.cpp --tone feedDone
tools/pet-studio/audio_tool.py c-array assets/work/audio/new-tone.json --name newTone
```

Profile-specific sprites and sounds should remain static build-time choices
generated from `profiles/*.json`; do not introduce runtime asset loading on
Arduboy.

The GUI can import Arduboy C arrays, ASCII grids, or PNG files; edit pixels;
flip, shift, crop, and slice frame strips; and export C arrays, ASCII grids, or
PNG previews. Exported C arrays use the current Arduboy convention in this
project: width and height first, followed by page-major bytes where `1` bits are
white/background and `0` bits are black pixels.

For multi-frame sprites, use `Copy frame` and `Paste frame` to carry a base
design between frames. `Paste black` overlays only the copied black pixels onto
the current frame, which is useful for keeping shared outlines while preserving
frame-specific fill or detail pixels.

## Review Checklist

Before committing sprite changes:

1. Run `nix develop -c make size`.
2. Compare flash/RAM against the previous roadmap baseline.
3. Run `nix develop -c make libretro-debug` for visual inspection.
4. Check the sprite on hardware before relying on fine 1-pixel details.
5. Update `docs/gameplay-audit.md` if controls, hitboxes, or behavior changed.
