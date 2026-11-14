#!/usr/bin/env python3
"""Import the vanilla 1.12 block PNGs from the user's MCP source ZIP parts.

Usage:
  python scripts/import_minecraft_textures.py "M1.12.2 part 2.zip"
  python scripts/import_minecraft_textures.py "M1.12.2 part 1.zip" "M1.12.2 part 2.zip"
"""
from pathlib import Path
from zipfile import ZipFile
import sys

FILES = {
    "dirt.png", "grass_top.png", "grass_side.png", "stone.png", "sand.png",
    "water_still.png", "bedrock.png", "log_oak.png", "leaves_oak.png",
    "gravel.png", "coal_ore.png", "iron_ore.png", "planks_oak.png",
    "crafting_table_side.png", "crafting_table_top.png", "crafting_table_front.png",
}
DEST = Path("assets/minecraft/textures/blocks")
PREFIX = "/src/minecraft/assets/minecraft/textures/blocks/"

def main():
    archives = [Path(arg) for arg in sys.argv[1:]]
    if not archives:
        raise SystemExit("Pass one or more MCP ZIP parts as arguments.")
    DEST.mkdir(parents=True, exist_ok=True)
    found = set()
    for archive in archives:
        with ZipFile(archive) as zf:
            for name in zf.namelist():
                if PREFIX not in name or name.rsplit("/", 1)[-1] not in FILES:
                    continue
                filename = name.rsplit("/", 1)[-1]
                if filename in found:
                    continue
                data = zf.read(name)
                if data.startswith(b"\x89PNG\r\n\x1a\n"):
                    (DEST / filename).write_bytes(data)
                    found.add(filename)
    missing = sorted(FILES - found)
    print(f"Imported {len(found)}/{len(FILES)} vanilla block textures into {DEST}")
    if missing:
        print("Missing:", ", ".join(missing))
        raise SystemExit(1)

if __name__ == "__main__":
    main()
