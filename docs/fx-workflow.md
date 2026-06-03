# Arduboy FX Workflow

Pocket Pixel defaults to an Arduboy FX-safe workflow. `make upload` refuses to
write the main sketch when `TARGET=fx`, because that is not the same as adding a
game to the FX catalog.

## Build the game

```sh
nix develop
make setup
make compile
```

The generated HEX is:

```text
build/pocket-pixel.ino.hex
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
```

This creates:

```text
dist/fx-cart/99-Development/01-Pocket-Pixel.hex
dist/fx-cart/99-Development/01-Pocket-Pixel.png
```

The PNG comes from the selected personality profile's `assets.banner` value.
The default profile is `profiles/pixel.json`.

This is only an entry to merge into an existing flashcart image or flashcart
directory. Do not write a one-game image to the FX chip unless you intend to
replace the full catalog.

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

## Classic Arduboy upload

Only use this for a non-FX Arduboy, or when you intentionally want to overwrite
the currently loaded sketch:

```sh
make upload TARGET=classic PORT=/dev/ttyACM0 CONFIRM_OVERWRITE=1
```
