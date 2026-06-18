# Pocket Pixel

Arduboy virtual pet sketch recovered from an older Arduino project.

## Development

Enter the reproducible development shell:

```sh
nix develop
```

Install the Arduino AVR core and libraries into local project directories:

```sh
make setup
```

Build the Arduboy/Leonardo hex:

```sh
make compile
```

Build the debug variant:

```sh
make compile-debug
```

Build the Arduboy FX-C variant reserved for future link-cable work:

```sh
make compile-fxc
```

Inspect AVR memory usage for the stable or debug build:

```sh
make size
make size-debug
make size-fxc
```

List symbols ordered by size when looking for RAM or flash targets:

```sh
make symbols
make symbols-debug
```

Open the project-local asset editor:

```sh
make pet-studio
```

The legacy alias still works:

```sh
make sprite-studio
```

This starts the project-local Pet Studio server. It loads profiles from
`profiles/`, sprites from `src/Assets.cpp`, and sounds from `src/Assets.cpp`.
It can save edited sprite drafts under `assets/work/sprites/` and audio drafts
under `assets/work/audio/`.
Stop the server with `Ctrl-C` when finished.

Extract Arduboy bitmap assets from `src/Assets.cpp` into JSON for inspection:

```sh
make sprites-json
```

Extract ArduboyTones sequences from `src/Assets.cpp` into JSON for inspection:

```sh
make audio-json
```

Print the generated hex path for an emulator:

```sh
make sim
```

Run locally with RetroArch and the Ardens libretro core from the flake:

```sh
make libretro
```

Run the debug variant locally:

```sh
make libretro-debug
```

In debug builds, press `A+B+Up` to toggle the debug menu. Use up/down to choose
an option, left/right to edit counters or flags, hold `B` while pressing
left/right for larger counter steps, `B` to run command rows, and `A` to exit.
The stable build does not include the debug menu.

The local RetroArch target appends `config/retroarch/arduboy-clean.cfg` to force
integer scaling, disable smoothing, and disable shaders. This keeps the Arduboy
1-bit framebuffer from being blended into gray fringes by emulator scaling.

Open Arduboy Cloud to test the generated hex with Ardens:

```sh
make cloud
```

Prepare the static web player used by GitHub Pages:

```sh
make web-site
```

This compiles the stable HEX, copies it to `site/build/`, and fetches the
Ardens web player files into `site/vendor/ardens/` for the local Pages preview
or CI artifact. The fetched Ardens binaries are generated files and are not
tracked in git.

The project defaults to an Arduboy FX-safe workflow. `make upload` refuses to
overwrite the sketch when `TARGET=fx`. See `docs/fx-workflow.md`.

Prepare a catalog entry for later merge into an FX flashcart:

```sh
make fx-entry
```

Prepare the matching Arduboy FX-C catalog entry:

```sh
make fx-entry-fxc
```

Build a distributable `.arduboy` package:

```sh
make package-arduboy ARDUBOY_VERSION=v0.2.0
```

## Web Player

The GitHub Pages build serves `site/`, a custom Arduboy shell around the Ardens
web player. Keyboard controls are:

- Arrow keys: D-pad.
- `Z` or `A`: Arduboy A button.
- `X`, `S`, or `B`: Arduboy B button.
- `M`: mute or unmute audio.

The on-screen controls also forward input to the embedded player. Browsers
block WebAudio autoplay until the user interacts with the page. If audio is
silent after loading, click or tap the game screen or one of the on-screen
controls once.

The shell includes decorative red/blue LED lenses. Ardens emulates Arduboy LEDs
internally, but the current `ArdensPlayer` web build does not expose LED state
to embedding pages, so the shell cannot mirror `digitalWriteRGB()` yet. Upstream
feature request: https://github.com/tiberiusbrown/Ardens/issues/147.

## Releases

GitHub Actions builds the stable HEX, debug HEX, FX-C HEX, Arduboy FX catalog
entry, FX-C catalog entry, and `.arduboy` package on pushes and pull requests.
Pushes to `main` publish the GitHub Pages web player.

Create a public GitHub Release by pushing a version tag:

```sh
git tag v0.1.0
git push origin v0.1.0
```

The release workflow uploads:

- `pocket-pixel-<tag>.hex`: stable Arduboy build.
- `pocket-pixel-debug-<tag>.hex`: debug build with the debug menu enabled.
- `pocket-pixel-fxc-<tag>.hex`: FX-C build reserved for link-cable support.
- `pocket-pixel-<tag>.arduboy`: package for loaders/emulators that accept the
  Arduboy package format.
- `pocket-pixel-fx-entry-<tag>.tar.gz`: FX catalog entry with HEX and banner.
- `pocket-pixel-fxc-entry-<tag>.tar.gz`: FX-C catalog entry with HEX and banner.
- `SHA256SUMS-<tag>.txt`: checksums for release artifacts.

For Arduboy FX, merge the FX catalog entry into a backed-up flashcart image.
Do not upload it as a classic Arduboy sketch unless you intentionally want to
overwrite the currently loaded sketch.

## License

Pocket Pixel is copyleft.

- Code, tools, build scripts, and project-local skills are licensed under
  `GPL-3.0-or-later`.
- Artwork, sprite drafts, audio, personality profiles, FX banners, and
  documentation are licensed under `CC-BY-SA-4.0`.
- Public forks and modified builds must credit the original project:
  `Pocket Pixel by Jujuyeh, https://github.com/Jujuyeh/pocket-pixel`.
- Modified builds must clearly indicate that changes were made and must not
  imply they are official Pocket Pixel releases unless approved.

See `LICENSE`, `LICENSES/README.md`, and `COPYRIGHT.md`.

## Layout

```text
pocket-pixel/
|-- pocket-pixel.ino              Arduino sketch entrypoint
|-- Makefile                      Build, emulator, FX entry, and tooling tasks
|-- flake.nix                     Reproducible Nix development shell
|-- .github/workflows/
|   `-- ci-release.yml            CI and GitHub Release automation
|-- assets/
|   |-- fx/
|   |   `-- banner.png            Arduboy FX catalog banner
|   `-- work/
|       |-- sprites/              Pet Studio sprite drafts and exported headers
|       `-- audio/                Pet Studio audio drafts
|-- config/
|   `-- retroarch/
|       `-- arduboy-clean.cfg     Clean 1-bit libretro display profile
|-- docs/
|   |-- codebase.md               Code and tooling architecture guide
|   |-- fx-workflow.md            Arduboy FX packaging safety workflow
|   |-- gameplay-audit.md         Current behavior and save format notes
|   |-- minigames.md              Minigame design and validation notes
|   |-- roadmap.md                Development plan
|   `-- sprite-guidelines.md      Sprite workflow and pixel-art handoff
|-- profiles/
|   `-- pixel.json                Active pet personality source profile
|-- site/                         GitHub Pages web player shell
|-- skills/                       Project-local agent workflows and scripts
|-- src/
|   |-- Assets.*                  Sprites, bitmap data, and tone sequences
|   |-- Debug.*                   Debug menu behind POCKET_PIXEL_DEBUG
|   |-- Game.cpp                  Main game loop, scenes, and minigames
|   |-- Personality.h             Active generated personality include
|   |-- Pet.*                     Pet state, flags, XP, life, and money
|   |-- PocketPixelGame.h         Public setup/loop API used by the sketch
|   |-- Render.*                  HUD, menu, and shared drawing helpers
|   |-- Save.*                    Versioned EEPROM persistence
|   `-- generated/
|       `-- ActivePersonalityProfile.h
`-- tools/
    |-- package-arduboy.py        Release .arduboy package generator
    `-- pet-studio/               Profile-aware sprite/audio/profile editor
```

The project now has a reliable compile target, local libretro emulator workflow,
and a conservative FX packaging path. Further cleanup should focus on manual
playtesting, input/scene structure, and gameplay balance.

See `docs/codebase.md` for a fuller map of the game code, Pet Studio, data
generation, build pipeline, and release artifacts.
