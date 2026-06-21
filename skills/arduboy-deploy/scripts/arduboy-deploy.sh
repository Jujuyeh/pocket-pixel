#!/usr/bin/env bash
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"
DEFAULT_LOADER_DIR="${ARDUBOY_FLASHCART_LOADER_DIR:-/tmp/arduboy-flashcart-loader}"

usage() {
  cat <<'EOF'
Usage:
  arduboy-deploy.sh <command> [options]

Commands:
  compile             Compile stable/debug/FX-C build.
  classic-upload      Upload to a classic Arduboy using Arduino CLI/Makefile.
  fx-install-loader   Clone/update chrisdiana/arduboy-flashcart-loader.
  fx-backup           Back up an Arduboy FX flashcart.
  fx-decompile        Decompile an FX flashcart backup image.
  fx-build-image      Build an FX image by replacing one decompiled entry.
  fx-write-image      Write an FX image with verification.
  fx-update-entry     Compile, rebuild one FX entry, and write with verification.
  fxc-list-ports      List connected Arduboy/FX-C serial ports.
  fxc-backup          Back up one FX-C flashcart by explicit serial port.
  fxc-decompile       Decompile an FX-C flashcart backup image.
  fxc-build-image     Insert Pocket Pixel into an FX-C backup and rebuild image.
  fxc-write-image     Write an FX-C image by explicit serial port.

Common options:
  --project-root DIR  Project directory. Default: current directory.
  --build NAME        stable, debug, or fxc. Default: stable.
  --profile FILE      Personality profile. Default: profiles/pixel.json.
  --no-nix            Do not wrap make/python commands in nix develop.
  --yes               Required for write/upload operations.

Run a command with --help for command-specific examples.
EOF
}

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

need_value() {
  [ "$#" -ge 2 ] || die "missing value for $1"
}

abs_path() {
  local path="$1"
  if [ -d "$path" ]; then
    (cd "$path" && pwd)
  else
    local dir
    dir="$(dirname "$path")"
    local base
    base="$(basename "$path")"
    (cd "$dir" && printf '%s/%s\n' "$(pwd)" "$base")
  fi
}

run_in_project() {
  local project_root="$1"
  local use_nix="$2"
  shift 2
  if [ "$use_nix" = "1" ] && [ -f "$project_root/flake.nix" ]; then
    (cd "$project_root" && nix develop -c "$@")
  else
    (cd "$project_root" && "$@")
  fi
}

profile_asset() {
  local project_root="$1"
  local use_nix="$2"
  local profile="$3"
  local asset="$4"
  local default="$5"
  python_cmd "$project_root" "$use_nix" \
    "$project_root/skills/pet-personality-profile/scripts/profile_tool.py" \
    asset "$profile" "$asset" --default "$default" | tail -n 1
}

python_cmd() {
  local project_root="$1"
  local use_nix="$2"
  shift 2
  if [ "$use_nix" = "1" ] && [ -f "$project_root/flake.nix" ]; then
    (cd "$project_root" && nix develop -c python "$@")
  elif command -v python3 >/dev/null 2>&1; then
    (cd "$project_root" && python3 "$@")
  elif command -v python >/dev/null 2>&1; then
    (cd "$project_root" && python "$@")
  else
    die "python not found; enter a dev shell or omit --no-nix for flake projects"
  fi
}

find_hex() {
  local project_root="$1"
  local build="$2"
  local hex
  hex="$(find "$project_root/build/$build" -maxdepth 2 -name '*.hex' ! -name '*with_bootloader*' -print -quit 2>/dev/null || true)"
  [ -n "$hex" ] || die "no .hex found under build/$build; compile first"
  printf '%s\n' "$hex"
}

loader_script() {
  local loader_dir="$1"
  local script="$2"
  local path="$loader_dir/Arduboy-Python-Utilities/$script"
  if [ ! -f "$path" ] && [ -f "/tmp/Arduboy-Python-Utilities/$script" ]; then
    path="/tmp/Arduboy-Python-Utilities/$script"
  fi
  [ -f "$path" ] || die "missing loader script: $path"
  printf '%s\n' "$path"
}

ensure_loader() {
  local loader_dir="$1"
  [ -d "$loader_dir/.git" ] || die "flashcart loader not found at $loader_dir; run fx-install-loader or pass --loader-dir"
}

command_compile() {
  local project_root="$PWD" build="stable" use_nix=1
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --build) need_value "$@"; build="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --help) printf 'Example: %s compile --build debug\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for compile: $1" ;;
    esac
  done
  project_root="$(abs_path "$project_root")"
  case "$build" in stable|debug|fxc) ;; *) die "--build must be stable, debug, or fxc" ;; esac
  if [ -f "$project_root/Makefile" ]; then
    case "$build" in
      debug) run_in_project "$project_root" "$use_nix" make compile-debug ;;
      fxc) run_in_project "$project_root" "$use_nix" make compile-fxc ;;
      stable) run_in_project "$project_root" "$use_nix" make compile ;;
    esac
  else
    die "no Makefile found; compile manually or add project-specific support"
  fi
}

command_classic_upload() {
  local project_root="$PWD" build="stable" port="" use_nix=1 yes=0
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --build) need_value "$@"; build="$2"; shift 2 ;;
      --port) need_value "$@"; port="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --yes) yes=1; shift ;;
      --help) printf 'Example: %s classic-upload --build stable --port /dev/ttyACM0 --yes\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for classic-upload: $1" ;;
    esac
  done
  [ "$yes" = "1" ] || die "classic upload overwrites the active sketch; pass --yes"
  [ -n "$port" ] || die "--port is required"
  project_root="$(abs_path "$project_root")"
  command_compile --project-root "$project_root" --build "$build" $([ "$use_nix" = "0" ] && printf '%s' --no-nix)
  run_in_project "$project_root" "$use_nix" make upload TARGET=classic BUILD="$build" PORT="$port" CONFIRM_OVERWRITE=1
}

command_fx_install_loader() {
  local loader_dir="$DEFAULT_LOADER_DIR"
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --loader-dir) need_value "$@"; loader_dir="$2"; shift 2 ;;
      --help) printf 'Example: %s fx-install-loader --loader-dir /tmp/arduboy-flashcart-loader\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fx-install-loader: $1" ;;
    esac
  done
  if [ -d "$loader_dir/.git" ]; then
    git -C "$loader_dir" pull --ff-only
    git -C "$loader_dir" submodule update --init --recursive
  else
    mkdir -p "$(dirname "$loader_dir")"
    git clone --recursive https://github.com/chrisdiana/arduboy-flashcart-loader "$loader_dir"
  fi

  local writer="$loader_dir/Arduboy-Python-Utilities/flashcart-writer.py"
  local verify="$loader_dir/Arduboy-Python-Utilities/flashcart-writer-verify.py"
  if [ -f "$writer" ] && [ ! -f "$verify" ]; then
    cp "$writer" "$verify"
  fi
}

command_fx_backup() {
  local project_root="$PWD" loader_dir="$DEFAULT_LOADER_DIR" output="" use_nix=1
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --loader-dir) need_value "$@"; loader_dir="$2"; shift 2 ;;
      --output) need_value "$@"; output="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --help) printf 'Example: %s fx-backup --output backups/fx/flashcart-backup.bin\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fx-backup: $1" ;;
    esac
  done
  [ -n "$output" ] || die "--output is required"
  project_root="$(abs_path "$project_root")"
  ensure_loader "$loader_dir"
  mkdir -p "$(dirname "$project_root/$output")"
  python_cmd "$project_root" "$use_nix" "$(loader_script "$loader_dir" flashcart-backup.py)" "$output"
}

command_fx_decompile() {
  local project_root="$PWD" loader_dir="$DEFAULT_LOADER_DIR" image="" use_nix=1
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --loader-dir) need_value "$@"; loader_dir="$2"; shift 2 ;;
      --image) need_value "$@"; image="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --help) printf 'Example: %s fx-decompile --image backups/fx/flashcart-backup.bin\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fx-decompile: $1" ;;
    esac
  done
  [ -n "$image" ] || die "--image is required"
  project_root="$(abs_path "$project_root")"
  ensure_loader "$loader_dir"
  python_cmd "$project_root" "$use_nix" "$(loader_script "$loader_dir" flashcart-decompiler.py)" "$image"
}

command_fx_build_image() {
  local project_root="$PWD" loader_dir="$DEFAULT_LOADER_DIR" build="debug"
  local backup_decompiled="" entry="" banner="" output_dir="" hex="" profile="profiles/pixel.json" use_nix=1
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --loader-dir) need_value "$@"; loader_dir="$2"; shift 2 ;;
      --build) need_value "$@"; build="$2"; shift 2 ;;
      --backup-decompiled) need_value "$@"; backup_decompiled="$2"; shift 2 ;;
      --entry) need_value "$@"; entry="$2"; shift 2 ;;
      --banner) need_value "$@"; banner="$2"; shift 2 ;;
      --profile) need_value "$@"; profile="$2"; shift 2 ;;
      --output-dir) need_value "$@"; output_dir="$2"; shift 2 ;;
      --hex) need_value "$@"; hex="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --help) printf 'Example: %s fx-build-image --build debug --backup-decompiled backups/fx/flashcart-backup --entry 3/6/6 --profile profiles/pixel.json\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fx-build-image: $1" ;;
    esac
  done
  [ -n "$backup_decompiled" ] || die "--backup-decompiled is required"
  [ -n "$entry" ] || die "--entry is required, for example 3/6/6"
  project_root="$(abs_path "$project_root")"
  ensure_loader "$loader_dir"
  if [ -z "$banner" ]; then
    banner="$(profile_asset "$project_root" "$use_nix" "$profile" banner assets/fx/banner.png)"
  fi
  [ -d "$project_root/$backup_decompiled" ] || [ -d "$backup_decompiled" ] || die "backup decompiled directory not found: $backup_decompiled"
  [ -f "$project_root/$banner" ] || [ -f "$banner" ] || die "banner not found: $banner"

  command_compile --project-root "$project_root" --build "$build" $([ "$use_nix" = "0" ] && printf '%s' --no-nix)
  if [ -z "$hex" ]; then
    hex="$(find_hex "$project_root" "$build")"
  fi
  [ -f "$hex" ] || [ -f "$project_root/$hex" ] || die "hex not found: $hex"
  [ -n "$output_dir" ] || output_dir="build/fx-work/flashcart-$build"

  local src_dir="$backup_decompiled"
  [ -d "$src_dir" ] || src_dir="$project_root/$backup_decompiled"
  local banner_path="$banner"
  [ -f "$banner_path" ] || banner_path="$project_root/$banner"
  local hex_path="$hex"
  [ -f "$hex_path" ] || hex_path="$project_root/$hex"
  local out="$project_root/$output_dir"

  rm -rf "$out"
  mkdir -p "$(dirname "$out")"
  cp -a "$src_dir" "$out"
  cp "$hex_path" "$out/$entry.hex"
  cp "$banner_path" "$out/$entry.png"
  python_cmd "$project_root" "$use_nix" "$(loader_script "$loader_dir" flashcart-builder.py)" "$output_dir/flashcart-index.csv"
  printf 'FX image: %s/flashcart-image.bin\n' "$out"
}

command_fx_write_image() {
  local project_root="$PWD" loader_dir="$DEFAULT_LOADER_DIR" image="" use_nix=1 yes=0
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --loader-dir) need_value "$@"; loader_dir="$2"; shift 2 ;;
      --image) need_value "$@"; image="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --yes) yes=1; shift ;;
      --help) printf 'Example: %s fx-write-image --image build/fx-work/flashcart-debug/flashcart-image.bin --yes\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fx-write-image: $1" ;;
    esac
  done
  [ "$yes" = "1" ] || die "FX write requires --yes"
  [ -n "$image" ] || die "--image is required"
  project_root="$(abs_path "$project_root")"
  ensure_loader "$loader_dir"
  local image_path="$image"
  [ -f "$image_path" ] || image_path="$project_root/$image"
  [ -f "$image_path" ] || die "image not found: $image"
  local writer
  writer="$(loader_script "$loader_dir" flashcart-writer-verify.py)"
  python_cmd "$project_root" "$use_nix" "$writer" "$image_path"
}

command_fx_update_entry() {
  local project_root="$PWD" loader_dir="$DEFAULT_LOADER_DIR" build="debug"
  local backup_decompiled="" entry="" banner="" output_dir="" hex="" profile="profiles/pixel.json" use_nix=1 yes=0
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --loader-dir) need_value "$@"; loader_dir="$2"; shift 2 ;;
      --build) need_value "$@"; build="$2"; shift 2 ;;
      --backup-decompiled) need_value "$@"; backup_decompiled="$2"; shift 2 ;;
      --entry) need_value "$@"; entry="$2"; shift 2 ;;
      --banner) need_value "$@"; banner="$2"; shift 2 ;;
      --profile) need_value "$@"; profile="$2"; shift 2 ;;
      --output-dir) need_value "$@"; output_dir="$2"; shift 2 ;;
      --hex) need_value "$@"; hex="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --yes) yes=1; shift ;;
      --help) printf 'Example: %s fx-update-entry --build debug --backup-decompiled backups/fx/flashcart-backup --entry 3/6/6 --profile profiles/pixel.json --yes\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fx-update-entry: $1" ;;
    esac
  done
  [ "$yes" = "1" ] || die "FX update requires --yes"
  [ -n "$output_dir" ] || output_dir="build/fx-work/flashcart-$build"

  local build_args=(
    --project-root "$project_root"
    --loader-dir "$loader_dir"
    --build "$build"
    --backup-decompiled "$backup_decompiled"
    --entry "$entry"
    --profile "$profile"
    --output-dir "$output_dir"
  )
  if [ -n "$banner" ]; then
    build_args+=(--banner "$banner")
  fi
  if [ -n "$hex" ]; then
    build_args+=(--hex "$hex")
  fi
  if [ "$use_nix" = "0" ]; then
    build_args+=(--no-nix)
  fi
  command_fx_build_image "${build_args[@]}"

  local write_args=(
    --project-root "$project_root"
    --loader-dir "$loader_dir"
    --image "$output_dir/flashcart-image.bin"
    --yes
  )
  if [ "$use_nix" = "0" ]; then
    write_args+=(--no-nix)
  fi
  command_fx_write_image "${write_args[@]}"
}

command_fxc_list_ports() {
  local project_root="$PWD" use_nix=1
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --help) printf 'Example: %s fxc-list-ports\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fxc-list-ports: $1" ;;
    esac
  done
  project_root="$(abs_path "$project_root")"
  python_cmd "$project_root" "$use_nix" "$project_root/tools/fxc-flash.py" list
}

command_fxc_backup() {
  local project_root="$PWD" port="" output="" use_nix=1
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --port) need_value "$@"; port="$2"; shift 2 ;;
      --output) need_value "$@"; output="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --help) printf 'Example: %s fxc-backup --port /dev/ttyACM0 --output backups/fxc/unit-a.bin\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fxc-backup: $1" ;;
    esac
  done
  [ -n "$port" ] || die "--port is required"
  [ -n "$output" ] || output="backups/fxc/flashcart-backup-image-$(date +%Y%m%d-%H%M%S)-$(basename "$port").bin"
  project_root="$(abs_path "$project_root")"
  mkdir -p "$(dirname "$project_root/$output")"
  python_cmd "$project_root" "$use_nix" "$project_root/tools/fxc-flash.py" backup --port "$port" --output "$output"
}

command_fxc_decompile() {
  local project_root="$PWD" loader_dir="$DEFAULT_LOADER_DIR" image="" use_nix=1
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --loader-dir) need_value "$@"; loader_dir="$2"; shift 2 ;;
      --image) need_value "$@"; image="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --help) printf 'Example: %s fxc-decompile --image backups/fxc/unit-a.bin\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fxc-decompile: $1" ;;
    esac
  done
  [ -n "$image" ] || die "--image is required"
  project_root="$(abs_path "$project_root")"
  ensure_loader "$loader_dir"
  python_cmd "$project_root" "$use_nix" "$(loader_script "$loader_dir" flashcart-decompiler.py)" "$image"
}

command_fxc_build_image() {
  local project_root="$PWD" loader_dir="$DEFAULT_LOADER_DIR" build="fxc"
  local backup_decompiled="" category="11" title="Pocket Pixel" banner="" output_dir="" hex="" profile="profiles/pixel.json" use_nix=1
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --loader-dir) need_value "$@"; loader_dir="$2"; shift 2 ;;
      --build) need_value "$@"; build="$2"; shift 2 ;;
      --backup-decompiled) need_value "$@"; backup_decompiled="$2"; shift 2 ;;
      --category) need_value "$@"; category="$2"; shift 2 ;;
      --title) need_value "$@"; title="$2"; shift 2 ;;
      --banner) need_value "$@"; banner="$2"; shift 2 ;;
      --profile) need_value "$@"; profile="$2"; shift 2 ;;
      --output-dir) need_value "$@"; output_dir="$2"; shift 2 ;;
      --hex) need_value "$@"; hex="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --help) printf 'Example: %s fxc-build-image --backup-decompiled backups/fxc/unit-a --category 11\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fxc-build-image: $1" ;;
    esac
  done
  [ -n "$backup_decompiled" ] || die "--backup-decompiled is required"
  project_root="$(abs_path "$project_root")"
  ensure_loader "$loader_dir"
  if [ -z "$banner" ]; then
    banner="$(profile_asset "$project_root" "$use_nix" "$profile" banner assets/fx/banner.png)"
  fi
  command_compile --project-root "$project_root" --build "$build" $([ "$use_nix" = "0" ] && printf '%s' --no-nix)
  if [ -z "$hex" ]; then
    hex="$(find_hex "$project_root" "$build")"
  fi
  [ -n "$output_dir" ] || output_dir="build/fxc-work/flashcart-$build"

  local src_dir="$backup_decompiled"
  [ -d "$src_dir" ] || src_dir="$project_root/$backup_decompiled"
  [ -d "$src_dir" ] || die "backup decompiled directory not found: $backup_decompiled"
  local banner_path="$banner"
  [ -f "$banner_path" ] || banner_path="$project_root/$banner"
  local hex_path="$hex"
  [ -f "$hex_path" ] || hex_path="$project_root/$hex"
  local out="$project_root/$output_dir"

  rm -rf "$out"
  mkdir -p "$(dirname "$out")"
  cp -a "$src_dir" "$out"
  python_cmd "$project_root" "$use_nix" "$project_root/tools/fxc-insert-entry.py" \
    --flashcart-dir "$out" \
    --category "$category" \
    --title "$title" \
    --hex "$hex_path" \
    --banner "$banner_path"
  python_cmd "$project_root" "$use_nix" "$(loader_script "$loader_dir" flashcart-builder.py)" "$output_dir/flashcart-index.csv"
  printf 'FX-C image: %s/flashcart-image.bin\n' "$out"
}

command_fxc_write_image() {
  local project_root="$PWD" port="" image="" use_nix=1 yes=0
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --project-root) need_value "$@"; project_root="$2"; shift 2 ;;
      --port) need_value "$@"; port="$2"; shift 2 ;;
      --image) need_value "$@"; image="$2"; shift 2 ;;
      --no-nix) use_nix=0; shift ;;
      --yes) yes=1; shift ;;
      --help) printf 'Example: %s fxc-write-image --port /dev/ttyACM0 --image build/fxc-work/flashcart-fxc/flashcart-image.bin --yes\n' "$SCRIPT_NAME"; return ;;
      *) die "unknown option for fxc-write-image: $1" ;;
    esac
  done
  [ "$yes" = "1" ] || die "FX-C write requires --yes"
  [ -n "$port" ] || die "--port is required"
  [ -n "$image" ] || die "--image is required"
  project_root="$(abs_path "$project_root")"
  local image_path="$image"
  [ -f "$image_path" ] || image_path="$project_root/$image"
  [ -f "$image_path" ] || die "image not found: $image"
  python_cmd "$project_root" "$use_nix" "$project_root/tools/fxc-flash.py" write --port "$port" --image "$image_path"
}

main() {
  [ "$#" -gt 0 ] || { usage; exit 0; }
  local command="$1"
  shift
  case "$command" in
    compile) command_compile "$@" ;;
    classic-upload) command_classic_upload "$@" ;;
    fx-install-loader) command_fx_install_loader "$@" ;;
    fx-backup) command_fx_backup "$@" ;;
    fx-decompile) command_fx_decompile "$@" ;;
    fx-build-image) command_fx_build_image "$@" ;;
    fx-write-image) command_fx_write_image "$@" ;;
    fx-update-entry) command_fx_update_entry "$@" ;;
    fxc-list-ports) command_fxc_list_ports "$@" ;;
    fxc-backup) command_fxc_backup "$@" ;;
    fxc-decompile) command_fxc_decompile "$@" ;;
    fxc-build-image) command_fxc_build_image "$@" ;;
    fxc-write-image) command_fxc_write_image "$@" ;;
    --help|-h|help) usage ;;
    *) die "unknown command: $command" ;;
  esac
}

main "$@"
