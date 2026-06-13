#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
site_dir="${root}/site"
hex_source="${1:-${root}/build/stable/pocket-pixel.ino.hex}"
ardens_base="${ARDENS_WEB_BASE:-https://tiberiusbrown.github.io/Ardens}"
ardens_license="${ARDENS_LICENSE_URL:-https://raw.githubusercontent.com/tiberiusbrown/Ardens/master/LICENSE.txt}"

if [[ ! -f "${hex_source}" ]]; then
  echo "Missing HEX: ${hex_source}" >&2
  echo "Run make compile first or pass the HEX path." >&2
  exit 1
fi

mkdir -p "${site_dir}/build" "${site_dir}/vendor/ardens"
cp "${hex_source}" "${site_dir}/build/pocket-pixel-latest.hex"
cp "${root}/assets/fx/banner.png" "${site_dir}/build/banner.png"

curl -fsSL "${ardens_base}/ArdensPlayer.js" \
  -o "${site_dir}/vendor/ardens/ArdensPlayer.js"
curl -fsSL "${ardens_base}/ArdensPlayer.wasm" \
  -o "${site_dir}/vendor/ardens/ArdensPlayer.wasm"
curl -fsSL "${ardens_license}" \
  -o "${site_dir}/vendor/ardens/LICENSE.txt"

cat > "${site_dir}/vendor/ardens/NOTICE.txt" <<'NOTICE'
Ardens web player files are fetched during the Pocket Pixel Pages build.

Ardens is copyright (c) 2023 Peter Brown and distributed under the MIT
License. See LICENSE.txt in this directory.

Source: https://github.com/tiberiusbrown/Ardens
NOTICE

echo "Prepared ${site_dir}"
