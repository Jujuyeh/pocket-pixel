---
name: arduboy-deploy
description: Compile, package, and upload Arduboy projects. Use when Codex needs to build stable/debug/FX-C Arduboy sketches, create .arduboy packages, upload to a classic Arduboy over Arduino CLI, or update an Arduboy FX/FX-C flashcart catalog entry from a backed-up/decompiled flashcart image without treating FX as a classic single-sketch upload target.
---

# Arduboy Deploy

Use this skill for Arduboy build/deploy tasks. Prefer the bundled script:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh --help
```

If the project has a Nix flake, run commands through `nix develop` unless the
user asks otherwise. For Pocket Pixel, the project Makefile already knows how to
build stable/debug/FX-C variants and package `.arduboy` releases.

## Safety Rules

- Never use classic Arduino upload for Arduboy FX or FX-C catalog updates.
- For Arduboy FX or FX-C, require a flashcart backup and a decompiled flashcart
  directory before writing.
- Keep classic FX and FX-C backup/decompile paths separate. Do not rebuild an
  FX-C image from a classic FX backup.
- Require an explicit catalog entry path, such as `3/6/6`, when replacing an FX
  game already in a catalog.
- Require `--yes` for write operations.
- Preserve the original backup image. Do not delete or commit it.
- If `/dev/ttyACM0` is not writable, check group/ACL. Prefer adding the user to
  `dialout` declaratively on NixOS; for a temporary session the user can run:

```sh
sudo setfacl -m u:$USER:rw /dev/ttyACM0
```

## Common Workflows

Compile stable:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh compile --build stable
```

Compile debug:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh compile --build debug
```

Compile FX-C:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh compile --build fxc
```

Create a `.arduboy` package:

```sh
make package-arduboy ARDUBOY_VERSION=dev
```

Upload to a classic Arduboy:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh classic-upload \
  --build stable \
  --port /dev/ttyACM0 \
  --yes
```

Install/update the external FX flashcart loader tools:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fx-install-loader
```

Back up an Arduboy FX flashcart:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fx-backup \
  --output backups/fx/flashcart-backup.bin
```

Back up an Arduboy FX-C flashcart:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-backup \
  --port /dev/ttyACM0 \
  --output backups/fxc/flashcart-backup.bin
```

When two FX-C units are connected, use the port-aware FX-C commands instead of
the original auto-detecting loader scripts:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-list-ports
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-backup \
  --port /dev/ttyACM0 \
  --output backups/fxc/pocket-pixel-unit-a.bin
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-decompile \
  --image backups/fxc/pocket-pixel-unit-a.bin
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-build-image \
  --backup-decompiled backups/fxc/pocket-pixel-unit-a \
  --category 11
skills/arduboy-deploy/scripts/arduboy-deploy.sh fxc-write-image \
  --port /dev/ttyACM0 \
  --image build/fxc-work/flashcart-fxc/flashcart-image.bin \
  --yes
```

Use `fxc-build-image` for first-time installation: it appends a new `Pocket
Pixel` entry to an existing category instead of replacing a numeric slot.

Decompile a backup image:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fx-decompile \
  --image backups/fx/flashcart-backup.bin
```

Build an updated FX image from a decompiled backup:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fx-build-image \
  --build debug \
  --backup-decompiled backups/fx/flashcart-backup \
  --entry 3/6/6 \
  --profile profiles/pixel.json
```

Write a prepared FX image:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fx-write-image \
  --image build/fx-work/flashcart-debug/flashcart-image.bin \
  --yes
```

One-step FX rebuild and write after the entry has already been identified:

```sh
skills/arduboy-deploy/scripts/arduboy-deploy.sh fx-update-entry \
  --build debug \
  --backup-decompiled backups/fx/flashcart-backup \
  --entry 3/6/6 \
  --profile profiles/pixel.json \
  --yes
```

## Pocket Pixel Defaults

For this Pocket Pixel repository, the currently known FX catalog entry is
`3/6/6`. The FX banner path should come from the active profile's
`assets.banner` value. Pixel currently uses `assets/fx/banner.png`. Do not
assume this entry for other projects or a different FX catalog image.

The FX-C target is separate:

```sh
make compile-fxc
make fx-entry-fxc
```

FX-C multiplayer is compiled behind `POCKET_PIXEL_FXC_LINK`; Pocket Pixel's
stable/debug builds must stay independent of any link-cable dependency.

The original full FX backup used during recovery was:

```text
backups/fx/flashcart-backup-image-20260524-114648.bin
```

Use the matching decompiled directory when rebuilding the current catalog:

```text
backups/fx/flashcart-backup-image-20260524-114648
```

## Verification

After compilation, report flash/RAM usage from Arduino CLI.

After FX writes, report:

- detected serial device,
- flash JEDEC/manufacturer/capacity,
- number of blocks written,
- final success or failure.

If an FX write fails after blocks have been written, stop and explain that the
backup image may need to be restored with `fx-write-image --image <backup.bin>
--yes`.
