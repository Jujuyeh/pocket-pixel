# Pocket Pixel Roadmap

This document is the living plan for Pocket Pixel. Keep it updated whenever a
cycle changes code, tooling, gameplay, save data, or Arduboy FX packaging.

## Current Baseline

- Build target: Arduboy/Leonardo through Arduino CLI.
- Dev environment: `nix develop`.
- Local emulator: RetroArch with the Ardens libretro core packaged by the flake.
- Local RetroArch profile: `config/retroarch/arduboy-clean.cfg` keeps scaling
  integer and disables shaders/smoothing for a clean 1-bit image.
- FX policy: guarded by default; catalog entries are generated separately.
- FX-C policy: separate `BUILD=fxc` and `make fx-entry-fxc` path for the two
  Arduboy FX-C units, reserved for future USB-C link-cable multiplayer.
- Main entrypoint: `pocket-pixel.ino`.
- Main loop and gameplay flow: `src/Game.cpp`.
- Active personality interface: `src/Personality.h`.
- Active generated personality data: `src/generated/ActivePersonalityProfile.h`.
- Assets: `src/Assets.*`.
- Rendering helpers: `src/Render.*`.
- Pet state: `src/Pet.*`.
- Save/load: `src/Save.*`.
- Debug tooling: `src/Debug.*`, compiled in only with `BUILD=debug`.
- Pet Studio tooling: `tools/pet-studio/`.
- Project-local automation skills: `skills/arduboy-deploy`,
  `skills/audio-workflow`, `skills/pet-personality-profile`, and
  `skills/sprite-workflow`.
- Current behavior notes: `docs/gameplay-audit.md`.
- Care minigame design: `docs/minigames.md`.

Last measured stable build:

```text
Sketch uses 24022 bytes (83%) of program storage space.
Global variables use 1561 bytes (60%) of dynamic memory.
```

Last measured debug build:

```text
Sketch uses 24700 bytes (86%) of program storage space.
Global variables use 1605 bytes (62%) of dynamic memory.
```

Last measured FX-C build:

```text
Sketch uses 27750 bytes (96%) of program storage space.
Global variables use 1767 bytes (69%) of dynamic memory.
```

Size tooling:

```sh
nix develop -c make size
nix develop -c make size-debug
nix develop -c make size-fxc
nix develop -c make symbols
```

All builds use AVR size-oriented compiler/linker flags shared with
All Men's Morris: `-mcall-prologues`, `-fno-inline-small-functions`, and
`-Wl,--relax`. On the FX-C build these recover roughly 1.2 KB of flash compared
with the same source compiled without them.

## Documentation Cycle

For each meaningful development cycle:

1. Update this roadmap before or during the work.
2. Update `docs/gameplay-audit.md` when behavior, controls, stats, or save data
   change.
3. Update `README.md` when user-facing commands or project layout change.
4. Update `AGENTS.md` when repo conventions or architecture expectations change.
5. Run verification commands and record relevant memory changes in the commit or
   final notes.

## Completed

- [x] Initialized a Git repo and pushed to `main`.
- [x] Removed the duplicate recovered sketch.
- [x] Added Nix flake development environment.
- [x] Added Arduino CLI build workflow.
- [x] Added local RetroArch/Ardens libretro workflow.
- [x] Added conservative Arduboy FX catalog-entry workflow.
- [x] Moved FX banner into `assets/fx/banner.png`.
- [x] Audited the recovered gameplay.
- [x] Added `AGENTS.md`.
- [x] Added `docs/gameplay-audit.md`.
- [x] Split pet state into `src/Pet.*`.
- [x] Added versioned EEPROM persistence in `src/Save.*`.
- [x] Replaced floating XP progress with tenths to avoid AVR float math.
- [x] Fixed status flag clearing so one cleared status no longer clears all
  statuses.
- [x] Implemented minimal care screens for play, clean, walk, and water.
- [x] Split sprite, bitmap, and tone data into `src/Assets.*`.
- [x] Split HUD, menu, and button hints into `src/Render.*`.
- [x] Replaced menu cursor Y-position checks with action indexes.
- [x] Added a clean-pixel RetroArch profile to avoid grayscale/blending
  artifacts during local playtesting.
- [x] Changed rendering to clear the framebuffer to white every frame and redraw
  the visible scene deterministically.
- [x] Added stable/debug build selection and a debug-only menu for forcing pet
  states, resources, and save reset.
- [x] Added project-local Arduboy deploy skill and scripts.
- [x] Added project-local pet personality profile skill and profile tooling.
- [x] Added a Pixel personality profile under `profiles/pixel.json`.
- [x] Wired the game to consume the active generated personality profile.
- [x] Documented the planned care minigames.
- [x] Implemented the first clean/litter-box minigame with free cursor canvas
  interaction and hidden coin bonuses.
- [x] Made hunger a derived condition from life instead of a separate effective
  status flag.
- [x] Improved the debug menu so counters and flags show current values and can
  be edited in place.
- [x] Added AVR size/symbol inspection targets for memory optimization work.
- [x] Moved fixed tone sequences to program memory, recovering 66 bytes of RAM.
- [x] Packed clean minigame flags and derived collected poop count from cleared
  pixels.
- [x] Tightened the clean tool overlay backup to the current sprite bounds,
  recovering 34 bytes of RAM.
- [x] Replaced duplicated sleep `Z` bitmap frames with procedural drawing,
  reducing flash while preserving the animation.
- [x] Added sprite collaboration guidelines in `docs/sprite-guidelines.md`.
- [x] Added a project-local sprite GUI and CLI for inspecting, editing,
  slicing, and converting Arduboy 1-bit bitmap sprites.
- [x] Cropped both pet idle frames from 32x32 to 26x24 and adjusted draw
  positions to preserve the same on-screen placement, saving 100 bytes of flash.
- [x] Implemented the feed minigame with falling fish/chicken, poop hazards,
  bowl fill states, profile-based food preference, and simple tone feedback.
- [x] Reworked feed progress so the bowl maps to life, early exit applies
  partial progress, poop halves progress, and completion restores full life.
- [x] Tuned Feed after hardware playtest: slightly faster falling food,
  slightly more poop, and shared the completion tone across successful care
  actions.
- [x] Raised the game loop to 30 FPS while scaling legacy 10 FPS timings and
  using subpixel movement where needed to preserve behavior with smoother
  rendering.
- [x] Implemented the Play timing minigame with a peeking finger, one attempt
  per cycle, paw feedback, and four successful swats to clear `BORED`.
- [x] Reworked Play around the intended hand-withdrawal timing: user-drawn
  background/hand/cat assets, bottom-left cat frame anchoring, 10-point goal,
  and scratch penalties with flash feedback.
- [x] Added editable Play cat masks, moved the cat 5 pixels left, and inverted
  the hand mechanic so the hand is out by default and B retracts it.
- [x] Tuned Play after sprite review: moved hand/cat left, reduced the target
  to 6, and made pounce timing randomized from the active profile's
  `playfulness`.
- [x] Added editable masks for the Play hand and adjusted its position 1 pixel
  left and 3 pixels up.
- [x] Implemented the Water spray minigame with a draining progress bar,
  randomized D-pad prompt, anti-multi-button spam rule, spray feedback, and
  red/blue LED siren while `SCRATCHING` is active.
- [x] Hardware-tested Water and accepted the current behavior.
- [x] Implemented the Walk laser-lane minigame with animated dashed lanes,
  smooth laser/cat lane movement, 12x12 obstacle sprites, score reset on hit,
  and completion at 16 passed obstacles.
- [x] Hardware-tested Walk and accepted the current behavior.
- [x] Added a GitHub Pages web player and release pipeline for public builds.

## Current Cycle: FX-C Linked Play

Goal: grow the FX-C visit flow into a small linked-play layer while keeping the
normal Arduboy/FX builds unchanged.

1. [x] Add `BUILD=fxc`, `make compile-fxc`, and `make size-fxc`.
2. [x] Add `make fx-entry-fxc` under `dist/fx-cart/FX-C/...` so FX-C catalog
   entries cannot be confused with classic FX entries.
3. [x] Add a release-ready `.arduboy` package generator.
4. [x] Update CI release artifacts to include FX-C HEX, FX-C entry tarball, and
   `.arduboy`.
5. [x] Document that FX and FX-C backups are separate and not interchangeable.
6. [x] Measure stable/debug/FX-C memory on this branch and decide how much room
   remains for one more minigame or a first multiplayer slice.
7. [x] Design and implement the first Pocket Pixel link protocol slice for Air
   Hockey.
8. [x] Add linked pet visit data exchange, guest arrival, linked menu, and
   conversation topics.
9. [x] Add linked game selection so the visit menu can request different
   minigames over the cable.
10. [x] Add Water Battle as a lightweight linked race using the Water spray
   meter.
11. [ ] Hardware-test Air Hockey on two FX-C units: linked idle wake, invite flow,
   host/client orientation, puck crossing, score sync, and disconnect behavior.
12. [ ] Hardware-test Water Battle on two FX-C units: menu routing, winner sync,
   hold-to-exit, and return-to-visit behavior.
13. [ ] Implement Food Rush once there is enough flash budget for the dropper
   and bowl roles.
14. [ ] Add graceful link-loss recovery during an active linked session: fade to
   white and restart the game state without replaying the boot sequence.

Nix follow-up notes:

- `make setup` now installs the pinned `ArduboyI2C` revision needed for future
  FX-C link builds. This still happens through Arduino CLI network setup.
- A future Nix hardening pass should move Arduino core/library provisioning out
  of ad-hoc `make setup`, or wrap it as fixed-output derivations, before we rely
  on FX-C multiplayer in CI.
- No new runtime Nix package is needed for `.arduboy` packaging; it uses Python
  standard-library `zipfile`.

Space expectation after measurement:

- Stable currently leaves 4,650 bytes of flash and 999 bytes of RAM.
- Debug leaves 3,972 bytes of flash and 955 bytes of RAM.
- FX-C with Ball Hunt, Water Battle, invite confirmation, visit-data exchange,
  host-side remote pet drawing, visit menu, conversations, and profile-driven
  meows leaves 922 bytes of flash and 793 bytes of RAM.
- More FX-C gameplay now requires careful budgeting; avoid large bitmaps and
  broad abstractions.
- The FX-C build keeps the custom boot animation, but uses a procedural Play
  background to reserve enough flash for ArduboyI2C and Air Hockey.
- The FX-C build disables USB CDC inside the sketch with `CDC_DISABLED`. The
  game is launched from the flashcart and does not use Serial at runtime; this
  recovers enough flash/RAM for the visit layer.

Verification:

```sh
nix flake check
nix develop -c make compile
nix develop -c make compile-debug
nix develop -c make compile-fxc
nix develop -c make package-arduboy ARDUBOY_VERSION=dev
```

Expected commit type:

```text
build: add fxc release target
```

## Completed Cycle: Clean Polish And Main UI

Goal: polish the existing care surface before adding deeper personality
systems, starting with the Clean minigame and then the main-screen UI.

1. [x] Close Walk as hardware-validated.
2. [x] Remove the placeholder rake movement sound after playtest showed it felt
   too percussive and repetitive.
3. [x] Make Clean poop patches a little more elongated.
4. [x] Draw a 1-pixel white readability border around revealed Clean poop
   pixels.
5. [x] Add a project-local audio helper for listing, extracting, and converting
   ArduboyTones arrays.
6. [x] Evolve the asset editor into Pet Studio with profile selection,
   profile creation/editing, sprite mode, and audio mode.
7. [x] Add a compact Pet Studio profile bar and a piano-roll audio view
   alongside the event-list editor.
8. [x] Split profile editing into a Preferences mode and fix Roll/Events
   visibility.
9. [x] Add Form/Raw submodes to Preferences and expand piano-roll timing scale.
10. [x] Update the main-screen menu into a mostly hidden right-side drawer with
   left/right open-close controls.
11. [x] Recenter the main HUD when the menu is hidden and show money in the top
   right with a `$` marker.
12. [ ] Keep deeper personality-profile expansion for a later cycle after the
   core game surface is stable.

Verification:

```sh
nix develop -c make compile
nix develop -c make compile-debug
nix develop -c make libretro-debug
```

Expected commit type:

```text
polish: improve clean feedback
```

## Later Cycle: Sprite Workflow Adoption

Goal: use Pet Studio to speed up minigame asset work without guessing at
Arduboy bitmap encoding by hand.

1. Export the current important sprites as ASCII/JSON for review.
2. Use the GUI for the next new minigame sprites before touching gameplay code.
3. Add any temporary user art references under a tracked project path, such as
   `assets/work/`, when they are needed for collaboration.
4. Replace temporary art with final `src/Assets.*` data only after libretro
   visual checks.
5. Keep `make size` comparisons with each sprite change.

Verification:

```sh
nix develop -c make sprites-json
nix develop -c make size
nix develop -c make libretro-debug
```

Expected commit type:

```text
feat: add sprite studio tooling
```

## Later Cycle: Memory And Storage Optimization

Goal: reduce RAM and flash usage without changing gameplay behavior.

1. Use `make size` and `make symbols` before and after each optimization.
2. Review `PET_IDLE1`/`PET_IDLE2` and `poo1`/`poo2` for safe delta or
   procedural overlays.
3. Keep stable/debug compile checks after every step.

Verification:

```sh
nix develop -c make size
nix develop -c make size-debug
nix develop -c make compile
nix develop -c make compile-debug
```

Expected commit types:

```text
build: add avr size tooling
refactor: reduce clean minigame state
refactor: move tones to program memory
```

## Later Cycle: Care Minigames

Goal: replace placeholder care screens with short, cat-specific minigames.

1. Keep `docs/minigames.md` as the design source.
2. Add a reusable minigame state pattern.
3. Playtest `Clean` on libretro and FX by forcing `DIRTY` from the debug menu.
4. Tune hidden coin odds and base clean reward.
5. Playtest `Feed` on libretro and FX by lowering life from the debug menu.
6. Document controls, rewards, and behavior.
7. Compile stable/debug after each implementation step.
8. Keep `BUILD=stable` as the default release path.

Verification:

```sh
nix flake check
nix develop -c make compile
nix develop -c make libretro
```

Expected commit type:

```text
feat: add clean minigame
```

## Later Cycle: Main HUD And Sliding Menu Polish

Goal: make the main screen feel more deliberate while preserving fast Arduboy
interaction and clear 1-bit rendering.

Planned behavior:

1. The main action menu is closed by default.
2. Press left to open the menu; press right to close it.
3. The menu slides in from the right and slides back out smoothly.
4. When closed, keep a 3-pixel-wide strip of the menu visible as an affordance.
5. Center the pet and other main UI elements smoothly as the menu opens/closes.
6. Stretch/reposition the level progress bar with the same transition.
7. Show money in the top-right corner as a `$` icon plus a variable-width number.
8. Keep the money display robust for increasing digit counts.

Visual notes:

- Make the menu border slightly softer by omitting the outer corner pixels on
  the upper-left and lower-left corners.
- Add a 1-pixel-wide, 5-pixel-tall vertical grip line centered vertically just
  inside the menu's left edge.
- Keep the effect procedural; avoid storing a bitmap for the menu chrome unless
  later profiling proves it cheaper.

Implementation notes:

- This likely wants a small menu animation state: closed/opening/open/closing.
- Prefer integer easing or a short fixed-step animation; avoid float math.
- Keep input behavior explicit so left/right only controls the menu on the main
  screen and does not interfere with care minigames or debug controls.
- Update `docs/gameplay-audit.md` when implemented because controls and HUD
  behavior will change.

Verification:

```sh
nix develop -c make libretro-debug
nix develop -c make size
```

Expected commit type:

```text
feat: add sliding main menu
```

## Backlog

### 1. Input And Scene Structure

- Add `src/Input.*` only if it removes real complexity.
- Consider a small `Scene` enum and action metadata table.

### 2. Better Gameplay Balance

- Tune random event chances after playtesting.
- Introduce `profiles/` and a `Personality` module so behavior values come from
  a selected pet profile instead of hardcoded constants.
- Decide whether sleeping should clear itself naturally or require time.
- Decide whether scratching should still remove money over time.
- Add more readable reward feedback after care actions.

### 3. Save Format Hardening

- Document EEPROM address, version, and fields more explicitly.
- Add migration notes before changing `PetState`.
- Consider a manual reset command if save data gets stuck during testing.

### 4. Arduboy FX Packaging

- Keep `make upload` guarded for FX.
- Improve documentation for merging `dist/fx-cart` into an existing flashcart
  backup.
- Do not add direct FX write commands until the exact safe write/update process
  is documented and tested.

### 5. Polish

- Improve action screen art while staying within memory limits.
- Add small sounds for successful care actions.
- The default Arduboy boot logo is skipped and replaced with a lightweight
  custom `bootLogo24x12` animation. Only add boot sound if flash budget remains
  acceptable after gameplay work.

## Regular Checks

Run before committing behavior or build changes:

```sh
nix flake check
nix develop -c make compile
nix develop -c make compile-debug
```

Run for manual playtesting:

```sh
nix develop -c make libretro
nix develop -c make libretro-debug
```
