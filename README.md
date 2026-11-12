# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by Minecraft Java 1.12.2. Independent implementation; no Minecraft source code or proprietary assets are included.

## Current milestone: playable sandbox prototype
Launch the game:
```powershell
.\build\Release\MineCppClone.exe --3d --size 64 --seed 1122
```

### Controls
- WASD — walk
- Mouse — look around
- Space — jump
- Left Shift — run
- Left mouse button — break the targeted block
- Right mouse button — place the selected block
- 1–5 or mouse wheel — select hotbar slot
- Escape — release / recapture the mouse

### Implemented gameplay
- First-person camera and mouse look.
- Player walking, running, jumping and basic ground-height collision.
- A small generated voxel world with grass, dirt, stone, sand, bedrock and water.
- Target selection by a short ray from the crosshair.
- Breaking and placing blocks.
- Five-slot hotbar with keyboard and mouse-wheel selection.
- Rendering limited to a nearby area around the player.

## Existing foundations
- Seeded terrain generation.
- Block registry and 16 x 256 x 16 chunk storage.
- World block access across positive and negative chunk coordinates.
- Six language catalogs: English, Russian, Spanish, German, French and Simplified Chinese.
- Windows x64 GitHub Actions build and CTest workflow.

## Build and test
Requires Visual Studio 2022 Build Tools with C++ workload and CMake. CMake fetches raylib 5.5 during configuration.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\MineCppClone.exe --self-test
```

## Roadmap
1. Playable first-person block sandbox prototype
2. Robust player collision and block-face targeting
3. Chunk mesh generation and distance-based chunk streaming
4. Inventory, survival/creative modes, crafting and saving
5. Caves, ores, structures and richer biome generation
6. Cross-platform multiplayer

This is an early prototype, not a complete Minecraft 1.12.2 recreation. The current world is deliberately small and the player collision is a first pass. Windows builds are downloadable from GitHub Actions only after a successful workflow run.
