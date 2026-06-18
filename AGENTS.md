# AGENTS.md - Pocket Pixel

## Role

You are working on Pocket Pixel, an Arduboy virtual pet game recovered from an
old Arduino sketch. Help evolve it into a maintainable Arduboy/Arduboy FX
project while preserving working behavior unless a gameplay change is explicitly
requested.

## Project Context

- Target device: Arduboy / Arduboy FX.
- Local build system: Nix flake plus Arduino CLI.
- Local emulator: RetroArch with the Ardens libretro core packaged by the flake.
- Primary branch: `main`.
- Remote: `https://github.com/Jujuyeh/pocket-pixel.git`.
- Current sketch entrypoint: `pocket-pixel.ino`.
- Current game implementation: `src/Game.cpp`.
- Active personality profile interface: `src/Personality.h`.
- Generated active profile data: `src/generated/ActivePersonalityProfile.h`.
- Assets module: `src/Assets.*`.
- Debug module: `src/Debug.*`.
- Render module: `src/Render.*`.
- Pet state module: `src/Pet.*`.
- Save module: `src/Save.*`.
- FX banner asset: `assets/fx/banner.png`.
- Pet Studio tooling: `tools/pet-studio/`.
- Project-local skills: `skills/arduboy-deploy`, `skills/audio-workflow`,
  `skills/pet-personality-profile`, and `skills/sprite-workflow`.

## Ground Rules

- Prefer small, reversible commits using Conventional Commits.
- Keep changes focused on this game; do not modify system NixOS configuration
  from this repository.
- Preserve existing gameplay during refactors unless the task asks for behavior
  changes.
- Do not directly upload to an Arduboy FX as if it were a classic Arduboy.
- Do not add commands that overwrite an FX flashcart without an explicit
  confirmation gate and documented backup flow.
- Keep generated build outputs out of git.
- Keep project-specific skills, scripts, profile schemas, and profile tooling in
  this repository. Do not create project-owned tooling only under the user's
  home directory.
- Store pet personality profile JSON files under `profiles/`, not inside
  `skills/`.
- Store Pet Studio saved drafts under `assets/work/sprites/` until they are
  reviewed and integrated into `src/Assets.*`.
- Prefer ASCII in source and docs unless a file already has a reason to use
  non-ASCII text.

## Development Commands

Enter the development shell:

```sh
nix develop
```

Install Arduino core and libraries into local project directories:

```sh
make setup
```

Compile the game:

```sh
make compile
```

Compile the debug variant:

```sh
make compile-debug
```

Compile the FX-C variant:

```sh
make compile-fxc
```

Regenerate the active profile header after editing a profile:

```sh
make profile-header
```

Run locally with libretro:

```sh
make libretro
```

Run locally with debug menu enabled:

```sh
make libretro-debug
```

Debug controls:

- `A+B+Up` toggles the debug menu.
- Up/down select a row.
- Left/right edits counters and flags.
- Hold `B` while pressing left/right for larger counter steps.
- `B` toggles flag rows and applies command rows.
- `A` closes the menu.

Run standard checks:

```sh
nix flake check
nix develop -c make compile
```

Prepare an Arduboy FX catalog entry:

```sh
make fx-entry
```

Prepare an Arduboy FX-C catalog entry:

```sh
make fx-entry-fxc
```

Create a release-ready `.arduboy` package:

```sh
make package-arduboy ARDUBOY_VERSION=dev
```

Run the project-local deploy helper:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh --help
```

Run the project-local personality profile helper:

```sh
skills/pet-personality-profile/scripts/profile_tool.py --help
```

Open the project-local asset editor:

```sh
make pet-studio
```

The legacy sprite-focused alias also works:

```sh
make sprite-studio
```

Run the project-local sprite helper:

```sh
tools/pet-studio/sprite_tool.py --help
```

Run the project-local audio helper:

```sh
tools/pet-studio/audio_tool.py --help
```

## Arduboy Constraints

- Watch both program storage and dynamic memory after every compile.
- Keep persistent bitmap, font, and sound data in `PROGMEM`.
- Keep personality profile data static and generated from `profiles/*.json`.
- Avoid dynamic allocation in gameplay code.
- Keep frame work predictable; favor simple state machines.
- Use Arduboy2 patterns already present in the codebase unless there is a strong
  reason to change.
- Prefer short functions and explicit game states over hidden global coupling.
- Keep debug tooling behind `POCKET_PIXEL_DEBUG`; stable builds must not expose
  debug menus.
- Keep future FX-C link-cable work behind `POCKET_PIXEL_FXC_LINK`; stable and
  debug builds must compile without link dependencies or peer discovery work.
- Treat hunger as derived from life below `MAX_LIFE`, not as an independently
  persisted status flag.

## Expected Architecture Direction

The `.ino` file should remain a minimal entrypoint. The implementation should
move toward focused modules:

- `Game`: high-level loop, scene dispatch, and gameplay flow.
- `Personality`: active generated behavior profile.
- `Pet`: virtual pet state and stat changes.
- `Input`: button handling.
- `Render`: drawing helpers and screen rendering.
- `Assets`: sprites, icons, fonts, and sound data.
- `Debug`: build-time debug menu and test-only state forcing.
- `Save`: versioned EEPROM persistence.

`Pet`, `Save`, `Render`, `Assets`, and `Debug` already exist. `Input` and
scene/action metadata are future cleanup targets. Refactor in small steps and
compile between steps.

Future FX-C multiplayer should live in focused `src/Link.*` files with no-op
stubs for non-FX-C builds, following the shape tested in `../all-mens-morris`
without directly coupling Pocket Pixel's pet gameplay to that game's turn
protocol.

## FX Safety Policy

`make upload` is intentionally guarded for `TARGET=fx`. For an Arduboy FX,
generate catalog entries with `make fx-entry` and merge them into a backed-up
flashcart image. Treat flashcart writing as replacing the full FX catalog unless
the tooling proves it is doing a safe targeted update.

For Arduboy FX-C, use `make compile-fxc` and `make fx-entry-fxc`. Keep FX-C
backups under a separate path such as `backups/fxc/`; do not rebuild an FX-C
flashcart image from a classic FX backup.

## Documentation

- Update `README.md` when user-facing commands change.
- Update `docs/fx-workflow.md` when FX packaging or upload behavior changes.
- Update `docs/codebase.md` when module boundaries, tooling APIs, or release
  workflow behavior change.
- Update `docs/roadmap.md` when the development plan changes materially.
- Update `docs/gameplay-audit.md` when controls, behavior, stats, or save data
  change.
- Update `skills/` when reusable project automation or agent workflows change.
- Update `docs/sprite-guidelines.md` when sprite tooling, handoff rules, or
  sprite optimization findings change.
- Keep completed roadmap items distinct from the next actionable cycle.

## Commit And Verification

Before committing, normally run:

```sh
nix flake check
nix develop -c make compile
nix develop -c make compile-debug
nix develop -c make compile-fxc
```

Use commit messages such as:

```text
build: add local emulator workflow
docs: add project roadmap
refactor: split pet state module
feat: add pet persistence
fix: correct input handling
```
