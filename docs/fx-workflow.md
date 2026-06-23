# Arduboy FX Workflow

Pocket Pixel defaults to an Arduboy FX-safe workflow. `make upload` refuses to
write the main sketch when `TARGET=fx`, because that is not the same as adding a
game to the FX catalog.

Treat Arduboy FX-C as its own target. FX and FX-C backup images are not
interchangeable, even if the AVR sketch is mostly the same today. The FX-C path
is reserved for USB-C link-cable support and must use a fresh backup from the
exact connected FX-C unit.

## Build the game

```sh
nix develop
make setup
make compile
make compile-fxc
```

The generated HEX is:

```text
build/stable/pocket-pixel.ino.hex
build/fxc/pocket-pixel.ino.hex
```

## Test locally

The default local emulator path is RetroArch with the Ardens libretro core built
by this flake:

```sh
make libretro
```

Arduboy Cloud is still useful as a browser fallback. It uses Ardens and supports
`.hex` and `.arduboy` files:

```sh
make cloud
```

Load `build/pocket-pixel.ino.hex` in Arduboy Cloud's emulator.

## Prepare a catalog entry

```sh
make fx-entry
make fx-entry-fxc
```

This creates:

```text
dist/fx-cart/99-Development/01-Pocket-Pixel.hex
dist/fx-cart/99-Development/01-Pocket-Pixel.png
dist/fx-cart/FX-C/99-Development/01-Pocket-Pixel.hex
dist/fx-cart/FX-C/99-Development/01-Pocket-Pixel.png
```

The PNG comes from the selected personality profile's `assets.banner` value.
The default profile is `profiles/pixel.json`.

This is only an entry to merge into an existing flashcart image or flashcart
directory. Do not write a one-game image to the FX chip unless you intend to
replace the full catalog.

Keep FX-C backups under a separate path such as `backups/fxc/`. Do not decompile
or rebuild an FX-C catalog from a classic FX backup under `backups/fx/`.

## Port-aware FX-C install flow

The upstream flashcart scripts auto-detect the first Arduboy-compatible serial
port. That is unsafe when two FX-C units are connected at the same time. Pocket
Pixel therefore provides project-local wrappers that require explicit ports for
backup and write:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-list-ports
```

For each connected FX-C, back up the exact unit:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-backup \
  --port /dev/ttyACM0 \
  --output backups/fxc/pocket-pixel-unit-a.bin
```

Then decompile that backup:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-decompile \
  --image backups/fxc/pocket-pixel-unit-a.bin
```

For a first-time install, append Pocket Pixel to an existing category instead of
replacing an unknown slot:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-build-image \
  --backup-decompiled backups/fxc/pocket-pixel-unit-a \
  --category 3
```

The default `--category 3` matches the stock FX-C `ARCADE` category observed
during Pocket Pixel testing. On the first Arcade install this inserted the game
as `3/25/25`, after verifying that no `Pocket Pixel` entry already existed.
Inspect the decompiled title screens before changing this for a different
catalog.

Only after confirming the rebuilt image and backup path, write it back to the
same unit:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-write-image \
  --port /dev/ttyACM0 \
  --image build/fxc-work/flashcart-fxc/flashcart-image.bin \
  --yes
```

Repeat the backup/decompile/build/write flow separately for the second FX-C.

## Package a .arduboy file

```sh
make package-arduboy ARDUBOY_VERSION=v0.2.0
```

This creates:

```text
dist/release/pocket-pixel-v0.2.0.arduboy
```

The package is a ZIP-compatible `.arduboy` file containing `info.json`, the
stable HEX, and the active profile banner. It is intended for loaders,
emulators, and Arduboy Cloud-style workflows. FX and FX-C catalog updates still
use the catalog-entry tarballs and a backed-up flashcart image.

## Safe update policy

Before writing to an Arduboy FX:

1. Back up or export the current FX flashcart image.
2. Confirm where the old Pocket Pixel entry is in that image.
3. Replace only that entry, or add the new entry to a chosen development
   category.
4. Build a full replacement `.bin` from the updated catalog.
5. Write the full `.bin` only after verifying it contains the original catalog
   plus the updated Pocket Pixel entry.

The chrisdiana flashcart loader can generate an index, build a flashcart image,
and write that complete image to the Arduboy FX. Treat its write step as a full
FX flashcart replacement, not as a single-game patch.

## FX-C multiplayer planning

Pocket Pixel includes a first link-cable minigame in the FX-C build: Air
Hockey. It is compiled behind `POCKET_PIXEL_FXC_LINK` and does not affect stable
or debug builds. See `docs/fxc-air-hockey.md`.

Recommended next porting shape:

1. Keep using the All Men's Morris `ArduboyI2C` approach as a reference, but
   keep Pocket Pixel packets specific to pet minigames and visits.
2. Keep the link layer in focused `src/Link.*` files with no-op stable stubs.
3. Extend compact packets only when needed: protocol magic/version, nonce,
   packet kind, local pet summary, visit request/accept, and small interaction
   events.
4. Keep peer discovery recoverable so a single FX-C without a peer never freezes.
5. Measure `make size`, `make size-debug`, and `make size-fxc` before adding UI
   or minigame code.

The current FX-C build uses an FX-C-only tradeoff to fit: it keeps the custom
boot animation, but draws the Play background procedurally instead of storing
the full bitmap. Avoid adding large new bitmaps to FX-C without a matching
optimization.

## Classic Arduboy upload

Only use this for a non-FX Arduboy, or when you intentionally want to overwrite
the currently loaded sketch:

```sh
make upload TARGET=classic PORT=/dev/ttyACM0 CONFIRM_OVERWRITE=1
```
