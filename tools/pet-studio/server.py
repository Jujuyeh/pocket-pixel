#!/usr/bin/env python3
"""Local Pet Studio server with project-save endpoints."""

from __future__ import annotations

import argparse
import base64
import binascii
import json
import re
import sys
import webbrowser
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import audio_tool  # noqa: E402
import sprite_tool  # noqa: E402


ROOT = Path(__file__).resolve().parents[2]
ASSETS_SOURCE = ROOT / "src" / "Assets.cpp"
WORK_DIR = ROOT / "assets" / "work" / "sprites"
AUDIO_WORK_DIR = ROOT / "assets" / "work" / "audio"
PROFILES_DIR = ROOT / "profiles"
FX_ASSETS_DIR = ROOT / "assets" / "fx"


def safe_name(name: str) -> str:
    """Sanitize user-facing asset names into filesystem-safe draft names."""
    cleaned = re.sub(r"[^A-Za-z0-9_ -]+", "_", name).strip().replace(" ", "_")
    return cleaned or "sprite"


def safe_slug(name: str) -> str:
    """Sanitize profile names into stable lowercase slugs."""
    cleaned = re.sub(r"[^a-z0-9_-]+", "-", name.lower()).strip("-")
    return cleaned or "profile"


class SpriteStudioHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        # Serve the repository root so saved assets can be previewed by path.
        super().__init__(*args, directory=str(ROOT), **kwargs)

    def log_message(self, format: str, *args) -> None:
        print(f"pet-studio: {format % args}")

    def do_GET(self) -> None:
        if self.path == "/api/assets":
            # Merge source assets and work-in-progress draft headers into one list.
            assets = sprite_tool.parse_arrays(ASSETS_SOURCE)
            for draft in sorted(WORK_DIR.glob("*.h")):
                assets.extend(sprite_tool.parse_arrays(draft))
            self.send_json(
                {
                    "source": str(ASSETS_SOURCE.relative_to(ROOT)),
                    "count": len(assets),
                    "assets": assets,
                }
            )
            return
        if self.path == "/api/audio":
            # Source tones are read from Assets.cpp; drafts live in assets/work/audio.
            tones = audio_tool.parse_tones(ASSETS_SOURCE)
            drafts = []
            for draft in sorted(AUDIO_WORK_DIR.glob("*.json")):
                try:
                    payload = json.loads(draft.read_text(encoding="utf-8"))
                    payload["path"] = str(draft.relative_to(ROOT))
                    drafts.append(payload)
                except json.JSONDecodeError:
                    continue
            self.send_json(
                {
                    "source": str(ASSETS_SOURCE.relative_to(ROOT)),
                    "count": len(tones),
                    "tones": tones,
                    "drafts": drafts,
                }
            )
            return
        if self.path == "/api/profiles":
            # Profiles are raw JSON so Pet Studio can edit future keys without migrations.
            profiles = []
            for profile in sorted(PROFILES_DIR.glob("*.json")):
                try:
                    payload = json.loads(profile.read_text(encoding="utf-8"))
                    profiles.append(
                        {
                            "slug": profile.stem,
                            "path": str(profile.relative_to(ROOT)),
                            "name": payload.get("name", profile.stem),
                            "species": payload.get("species", "cat"),
                            "data": payload,
                        }
                    )
                except json.JSONDecodeError:
                    continue
            self.send_json({"count": len(profiles), "profiles": profiles})
            return
        super().do_GET()

    def do_POST(self) -> None:
        if self.path == "/api/save-sprite":
            self.save_sprite()
            return
        if self.path == "/api/save-audio":
            self.save_audio()
            return
        if self.path == "/api/save-profile":
            self.save_profile()
            return
        if self.path == "/api/save-banner":
            self.save_banner()
            return
        self.send_error(404)

    def read_payload(self) -> dict:
        """Read the current JSON POST body."""
        length = int(self.headers.get("Content-Length", "0"))
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def save_sprite(self) -> None:
        """Persist a sprite draft as both C source and ASCII review text."""
        try:
            payload = self.read_payload()
            name = safe_name(str(payload.get("name", "sprite")))
            c_source = str(payload.get("cSource", "")).strip()
            ascii_source = str(payload.get("asciiSource", "")).strip()
            if not c_source:
                raise ValueError("Missing C source")

            WORK_DIR.mkdir(parents=True, exist_ok=True)
            header_path = WORK_DIR / f"{name}.h"
            ascii_path = WORK_DIR / f"{name}.txt"
            header_path.write_text(c_source + "\n", encoding="utf-8")
            if ascii_source:
                ascii_path.write_text(ascii_source + "\n", encoding="utf-8")

            self.send_json(
                {
                    "ok": True,
                    "header": str(header_path.relative_to(ROOT)),
                    "ascii": str(ascii_path.relative_to(ROOT)) if ascii_source else None,
                }
            )
        except Exception as exc:  # pragma: no cover - surfaced to the browser
            self.send_json({"ok": False, "error": str(exc)}, status=400)

    def save_audio(self) -> None:
        """Persist an audio draft as JSON plus generated ArduboyTones C source."""
        try:
            payload = self.read_payload()
            name = safe_name(str(payload.get("name", "tone")))
            events = payload.get("events")
            if not isinstance(events, list) or not events:
                raise ValueError("Audio draft needs at least one event")
            draft = {"name": name, "events": events}
            c_source = audio_tool.c_array(name, events)

            AUDIO_WORK_DIR.mkdir(parents=True, exist_ok=True)
            json_path = AUDIO_WORK_DIR / f"{name}.json"
            header_path = AUDIO_WORK_DIR / f"{name}.h"
            json_path.write_text(json.dumps(draft, indent=2) + "\n", encoding="utf-8")
            header_path.write_text(c_source + "\n", encoding="utf-8")

            self.send_json(
                {
                    "ok": True,
                    "json": str(json_path.relative_to(ROOT)),
                    "header": str(header_path.relative_to(ROOT)),
                    "cSource": c_source,
                }
            )
        except Exception as exc:  # pragma: no cover - surfaced to the browser
            self.send_json({"ok": False, "error": str(exc)}, status=400)

    def save_profile(self) -> None:
        """Write a profile JSON file, preserving unknown profile fields."""
        try:
            payload = self.read_payload()
            data = payload.get("data")
            if not isinstance(data, dict):
                raise ValueError("Profile payload must contain a data object")
            name = str(data.get("name") or payload.get("name") or "Profile")
            slug = safe_slug(str(payload.get("slug") or name))
            data.setdefault("schemaVersion", 1)
            data.setdefault("species", "cat")
            data["name"] = name

            PROFILES_DIR.mkdir(parents=True, exist_ok=True)
            profile_path = PROFILES_DIR / f"{slug}.json"
            profile_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
            self.send_json(
                {
                    "ok": True,
                    "slug": slug,
                    "path": str(profile_path.relative_to(ROOT)),
                    "data": data,
                }
            )
        except Exception as exc:  # pragma: no cover - surfaced to the browser
            self.send_json({"ok": False, "error": str(exc)}, status=400)

    def save_banner(self) -> None:
        """Save a profile FX banner while constraining writes to assets/fx."""
        try:
            payload = self.read_payload()
            slug = safe_slug(str(payload.get("slug") or "profile"))
            path_value = str(payload.get("path") or f"assets/fx/{slug}-banner.png")
            data_url = str(payload.get("dataUrl") or "")
            if "," in data_url:
                data_url = data_url.split(",", 1)[1]
            if not data_url:
                raise ValueError("Missing banner image data")

            banner_path = (ROOT / path_value).resolve()
            assets_root = FX_ASSETS_DIR.resolve()
            # Pixel may keep the default banner path; other profiles get unique files.
            if path_value == "assets/fx/banner.png" and slug != "pixel":
                banner_path = assets_root / f"{slug}-banner.png"
            elif not banner_path.is_relative_to(assets_root):
                banner_path = assets_root / f"{slug}-banner.png"

            image = base64.b64decode(data_url, validate=True)
            if not image.startswith(b"\x89PNG\r\n\x1a\n"):
                raise ValueError("Banner must be a PNG image")

            banner_path.parent.mkdir(parents=True, exist_ok=True)
            banner_path.write_bytes(image)
            self.send_json(
                {
                    "ok": True,
                    "path": str(banner_path.relative_to(ROOT)),
                }
            )
        except (binascii.Error, ValueError) as exc:
            self.send_json({"ok": False, "error": str(exc)}, status=400)
        except Exception as exc:  # pragma: no cover - surfaced to the browser
            self.send_json({"ok": False, "error": str(exc)}, status=400)

    def send_json(self, payload: dict, status: int = 200) -> None:
        """Send a JSON response without pulling in a web framework."""
        body = json.dumps(payload, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8123)
    parser.add_argument("--open", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    url = f"http://{args.host}:{args.port}/tools/pet-studio/"
    server = ThreadingHTTPServer((args.host, args.port), SpriteStudioHandler)
    print(f"Pet Studio: {url}")
    print("Stop with Ctrl-C.")
    if args.open:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print()
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
