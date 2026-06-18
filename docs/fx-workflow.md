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

Pocket Pixel does not yet include link-cable gameplay. The build target is in
place so multiplayer can be added behind `POCKET_PIXEL_FXC_LINK` without
affecting stable or debug builds.

Recommended porting shape:

1. Reuse the All Men's Morris `ArduboyI2C` approach as a reference, but design a
   Pocket Pixel protocol around pet visits rather than turn-based moves.
2. Keep the link layer in focused `src/Link.*` files with no-op stable stubs.
3. Exchange compact packets: protocol magic/version, nonce, packet kind, local
   pet summary, visit request/accept, and small interaction events.
4. Keep peer discovery recoverable so a single FX-C without a peer never freezes.
5. Measure `make size`, `make size-debug`, and `make size-fxc` before adding UI
   or minigame code.

Initial expectation: there is enough stable-build flash for a small minigame or
first multiplayer slice, but not enough debug-build flash for careless shared
code. Prefer protocol stubs and one tiny interaction loop first, then optimize.

## Classic Arduboy upload

Only use this for a non-FX Arduboy, or when you intentionally want to overwrite
the currently loaded sketch:

```sh
make upload TARGET=classic PORT=/dev/ttyACM0 CONFIRM_OVERWRITE=1
```
