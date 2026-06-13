# Codebase Guide

This document maps the Pocket Pixel game code, local tools, generated data, and
release workflow. Use it as the first reference before changing behavior or
moving files.

## Runtime Overview

Pocket Pixel is an Arduboy virtual pet game. The Arduino sketch entrypoint is
kept intentionally small:

```text
pocket-pixel.ino
`-- setup() -> gameSetup()
`-- loop()  -> gameLoop()
```

The game runs through Arduboy2 at `GAME_FPS = 30`. Most legacy timing values
were originally tuned around 10 FPS, so helpers such as `framesAtGameFps()`
scale old frame counts to the current framerate.

Startup uses `arduboy.beginDoFirst()` and `arduboy.waitNoButtons()` instead of
`arduboy.begin()`. This preserves hardware, flashlight, system-button, and audio
initialization while skipping the default scrolling Arduboy boot logo. A custom
Pocket Pixel boot animation then runs before EEPROM load.

## Game Modules

```text
src/
|-- PocketPixelGame.h
|-- Game.cpp
|-- Assets.h
|-- Assets.cpp
|-- Pet.h
|-- Pet.cpp
|-- Save.h
|-- Save.cpp
|-- Render.h
|-- Render.cpp
|-- Debug.h
|-- Debug.cpp
|-- Personality.h
`-- generated/
    `-- ActivePersonalityProfile.h
```

### `PocketPixelGame.h`

Public sketch lifecycle API:

- `gameSetup()`: initializes Arduboy, display state, random seed, save data,
  personality chances, and the initial idle state.
- `gameLoop()`: advances one frame when Arduboy is ready, polls buttons,
  dispatches the current scene, updates LEDs, and displays the frame.

### `Game.cpp`

Main gameplay implementation. It currently owns the scene state machines and
most minigame-specific drawing/update code.

Core responsibilities:

- Global Arduboy/Tinyfont/ArduboyTones instances.
- Game state enum: `IDLE`, `FEED`, `PLAY`, `CLEAN`, `WALK`, `WATER`.
- Idle screen flow, action drawer, boot animation, state transitions, rewards,
  and save calls.
- Minigame state structs and logic:
  - `FeedMinigame`: bowl movement, falling food, falling poop, falling coins,
    fill-to-life conversion, and early exits.
  - `PlayMinigame`: hand timing, cat warning/lunge/strike phases, scoring, and
    scratch penalty.
  - `CleanMinigame`: litter framebuffer behavior, rake/scoop modes, hidden poop,
    hidden coins, sand repainting, and tool overlay backup.
  - `WalkMinigame`: lane scrolling, laser target, delayed cat follow, obstacles,
    coins, and score reset on collision.
  - `WaterMinigame`: spray spam challenge, target button changes, progress bar,
    scratch siren LED handling, and completion.

Important patterns:

- Keep minigame state in compact structs with fixed-size arrays.
- Avoid dynamic allocation during gameplay.
- Prefer small integer types where bounds are known.
- Keep persistent images/sounds in `PROGMEM`.
- Keep debug-only behavior behind `POCKET_PIXEL_DEBUG`.

Future cleanup target: split minigame update/render/input code out of
`Game.cpp` once behavior stabilizes and flash budget allows the refactor.

### `Assets.h` and `Assets.cpp`

Compile-time asset registry.

Contains:

- Pet sprites and status icons.
- Feed, play, water, walk, clean, and boot sprites.
- Legacy reference sprites kept for comparison.
- ArduboyTones tone arrays such as `denied`, `feedCatch`, `feedBad`,
  `feedDone`, `coinPickup`, and snoring sounds.
- Includes of reviewed Pet Studio sprite headers from `assets/work/sprites/`.

Sprite format:

```cpp
const uint8_t PROGMEM name[] = {
  width, height,
  // page-ordered Arduboy bitmap bytes
};
```

Sound format:

```cpp
const uint16_t PROGMEM name[] = {
  NOTE_C5, 50,
  NOTE_REST, 20,
  TONES_END
};
```

Do not include unreviewed drafts directly in `Assets.cpp`. Drafts should first
be saved in `assets/work/sprites/` or `assets/work/audio/`, reviewed visually or
audibly, then included intentionally.

### `Pet.h` and `Pet.cpp`

Owns the virtual pet state and stat mutation helpers.

State fields:

- `statusFlags`: bitset of `SLEEPING`, `HUNGRY`, `DIRTY`, `BORED`, `ANXIOUS`,
  and `SCRATCHING`.
- `life`: half-heart units, from `0` to `MAX_LIFE`.
- `level`: current level.
- `xpTenths`: XP progress in tenths.
- `money`: current money total.

Important behavior:

- Hunger is derived from `life < MAX_LIFE`; it should not be treated as an
  independently persisted status.
- Money clamps at zero on spending.
- XP increments in tenths and levels up when it reaches 10 tenths.

### `Save.h` and `Save.cpp`

Versioned EEPROM persistence.

Current format:

- EEPROM address: `128`.
- Magic: `0x5050`.
- Version: `1`.
- Payload: `PetState`.
- Integrity: XOR checksum over the serialized save payload.

Save behavior:

- Invalid or missing save data resets to `PetState()` defaults.
- The derived hungry flag is stripped before saving and after loading.
- Life and XP are sanity-clamped after load.

Before changing `PetState`, update save documentation and add migration notes.

### `Render.h` and `Render.cpp`

Shared HUD and menu drawing helpers.

Owns:

- Menu geometry constants.
- Life bar rendering.
- XP bar rendering with dynamic width.
- Money HUD positioning based on digit count.
- Right-side command drawer border and cursor rendering.
- Small button hints.

Keep generic HUD work here. Scene-specific drawing should stay with that scene
until a real repeated pattern emerges.

### `Debug.h` and `Debug.cpp`

Debug-only menu compiled when `POCKET_PIXEL_DEBUG` is set.

Controls:

- `A+B+Up`: toggle debug menu.
- Up/down: move row.
- Left/right: edit values.
- Hold `B` while editing: larger steps.
- `B`: toggle flags or apply command rows.
- `A`: close menu.

Stable builds return `false` from `debugHandleFrame()` and do not include the
debug menu UI.

### `Personality.h` and `generated/ActivePersonalityProfile.h`

`Personality.h` is a stable include wrapper for generated profile data.

`ActivePersonalityProfile.h` is generated from `profiles/*.json` by:

```sh
make profile-header
```

Generated fields currently include:

- Display name.
- Menu navigation melody.
- Playfulness.
- Random status chance thresholds.
- Feed cost.
- Fish/chicken preference weights.

Do not edit generated profile headers by hand. Edit JSON profiles, then
regenerate.

## Profiles

Profiles live under `profiles/`. The default profile is `profiles/pixel.json`.

Profiles drive personality and profile-specific assets:

- Pet name and species.
- Behavior/chance weights.
- Food preferences.
- Menu melody.
- FX banner path.

Pet Studio can edit profiles in form mode or raw JSON mode. The profile tool can
also inspect or regenerate profile-derived headers:

```sh
skills/pet-personality-profile/scripts/profile_tool.py --help
```

## Pet Studio

Pet Studio is the project-local browser editor for profiles, sprites, banners,
and ArduboyTones audio.

```text
tools/pet-studio/
|-- index.html       UI structure
|-- styles.css       Responsive editor layout and panels
|-- app.js           Browser-side editor state, drawing, audio, profile logic
|-- server.py        Local HTTP server and save APIs
|-- sprite_tool.py   Arduboy bitmap extraction/conversion CLI
`-- audio_tool.py    ArduboyTones extraction/conversion CLI
```

Launch it with:

```sh
make pet-studio
```

### Browser App

`app.js` owns three modes:

- `preferences`: profile selector, profile form editor, raw JSON editor, and FX
  banner preview/upload.
- `sprite`: pixel editor, frame navigation, draw/erase/toggle/line tools,
  brush size, grid, coordinate display, crop, shift, flip, slice, import/export,
  PNG export, draft save, and profile-banner sprite editing.
- `audio`: piano roll, event list, WebAudio preview, JSON export, C array
  export, and audio draft saving.

Sprite editor data model:

- A sprite is an array of frame objects.
- Each frame stores `width`, `height`, and a boolean pixel grid.
- `#` means a black Arduboy pixel in ASCII drafts.
- `.` means an unlit/white Arduboy pixel in ASCII drafts.

Audio editor data model:

- A sound is an event list of `{ note, duration }` entries.
- Notes use ArduboyTones note names or `NOTE_REST`.
- The piano roll converts visual notes into the same event list.

### Local Server

`server.py` serves the repository root and provides JSON APIs:

- `GET /api/assets`: returns sprites parsed from `src/Assets.cpp` and reviewed
  draft headers in `assets/work/sprites/`.
- `GET /api/audio`: returns tone arrays from `src/Assets.cpp` and JSON drafts in
  `assets/work/audio/`.
- `GET /api/profiles`: returns JSON profiles from `profiles/`.
- `POST /api/save-sprite`: writes `.h` and optional `.txt` sprite drafts under
  `assets/work/sprites/`.
- `POST /api/save-audio`: writes `.json` and `.h` audio drafts under
  `assets/work/audio/`.
- `POST /api/save-profile`: writes profile JSON under `profiles/`.
- `POST /api/save-banner`: writes PNG banners under `assets/fx/`.

The server is intentionally local-only by default (`127.0.0.1`). Do not expose it
to the network; it has write endpoints for project files.

### CLI Tools

`sprite_tool.py` supports:

- `extract`: parse Arduboy bitmap arrays into JSON.
- `list`: list assets.
- `ascii`: print one frame as `#`/`.` ASCII.
- `from-ascii`: convert an ASCII draft to a C array.
- `png-to-c`: convert a PNG to a C array.

`audio_tool.py` supports:

- `extract`: parse ArduboyTones arrays into JSON.
- `list`: list sounds.
- `c-array`: convert JSON/event data into a C array.

These tools are used by the Makefile and Pet Studio server. Prefer extending
them over creating one-off conversion scripts.

## Build And Release Pipeline

Local commands:

- `make setup`: installs Arduino AVR core and Arduboy libraries into local
  project directories.
- `make compile`: builds the stable HEX.
- `make compile-debug`: builds the debug HEX.
- `make fx-entry`: prepares `dist/fx-cart/...` with HEX and FX banner.
- `make web-site`: prepares the static GitHub Pages player under `site/`.
- `make libretro`: runs the stable build with RetroArch and Ardens.
- `make libretro-debug`: runs the debug build.

Web player notes:

- `site/index.html` draws the custom Pocket Pixel web shell and controller.
- `site/player.html` wraps ArdensPlayer and loads the generated HEX.
- `site/app.js` forwards keyboard and pointer input into the iframe.
- Audio mute state is handled by the wrapper. Browsers still require a user
  gesture before WebAudio can play. Avoid repeated suspend/resume workarounds
  because they can degrade playback on mobile browsers.
- The shell draws red/blue LED lenses, but the current ArdensPlayer web build
  does not expose emulated LED state to JavaScript. The LEDs are therefore
  decorative until upstream exposes `led_rgb()` or an equivalent callback.
  Tracking issue: https://github.com/tiberiusbrown/Ardens/issues/147.

GitHub Actions workflow:

```text
.github/workflows/ci-release.yml
```

On push and pull request:

- Installs Nix.
- Downloads Arduino CLI.
- Runs `nix flake check`.
- Runs `make setup`.
- Regenerates the active profile header.
- Compiles stable and debug builds.
- Prepares FX catalog entry.
- Uploads workflow artifacts.

On pushes to `main` and version tags:

- Prepares the static Pages site.
- Copies the stable HEX to `site/build/pocket-pixel-latest.hex`.
- Downloads Ardens web player files into the Pages artifact.
- Deploys GitHub Pages.

On tags matching `v*`:

- Publishes a GitHub Release.
- Uploads stable HEX, debug HEX, FX entry tarball, release README, and SHA256
  checksums.

## Licensing

Code, build scripts, tools, skills, and CI are `GPL-3.0-or-later`.

Creative assets, profile data, and documentation are `CC-BY-SA-4.0`.

See:

- `LICENSE`
- `LICENSES/README.md`
- `COPYRIGHT.md`

## Maintenance Guidelines

- Keep generated build outputs out of git.
- Commit reviewed source assets, not ad hoc local exports.
- Run `nix flake check`, `make compile`, and `make compile-debug` before
  behavior changes are pushed.
- Watch AVR flash and RAM after every compile; debug is close to the flash
  limit.
- Prefer deterministic procedural effects over large bitmaps when flash is
  tight.
- Keep FX writes conservative: generate catalog entries, then merge them into a
  backed-up flashcart image.
