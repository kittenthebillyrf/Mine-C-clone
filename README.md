# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by Minecraft Java 1.12.2. Independent implementation; no Minecraft source code or proprietary assets are included.

## Current milestone: playable sandbox prototype
Launch:
```powershell
.\build\Release\MineCppClone.exe --3d --size 64 --seed 1122
```

### Controls
- WASD — move; mouse — look
- Space — jump; Left Shift — run
- Left mouse — break the targeted block
- Right mouse — place one block from the selected stack
- 1–9 / mouse wheel — select hotbar slot
- E — open/close inventory
- In the inventory, click a hotbar slot to select it; click a storage slot to swap it with the selected hotbar slot
- Escape — release / recapture the mouse

### Implemented gameplay
- First-person camera and mouse look.
- Walking, running, jumping and basic height-based ground collision.
- Seeded block world containing grass, dirt, stone, sand, bedrock and water.
- Crosshair ray targeting, block breaking and placement.
- 9-slot hotbar and 36-slot inventory.
- Block stacks, stack limit of 64, pickup when breaking and consumption when placing.
- Nearby block rendering around the player.

## Existing foundations
- Block registry and 16 x 256 x 16 chunk storage.
- World block access across positive and negative chunk coordinates.
- Six language catalogs: English, Russian, Spanish, German, French and Simplified Chinese.
- Windows x64 GitHub Actions build and CTest workflow.

## Export a map image (optional)\n\nThe PPM terrain image is no longer the default launch mode. To create one explicitly, run:\n\n```powershell\n.\\build\\Release\\MineCppClone.exe --map --size 128 --seed 1122 --output terrain.ppm\n```\n\n## Build and test
Requires Visual Studio 2022 Build Tools with C++ workload and CMake. CMake fetches raylib 5.5 during configuration.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\MineCppClone.exe --self-test
```

## Next milestones
1. Reliable voxel collision and block-face targeting
2. Chunk mesh generation and distance-based chunk streaming
3. Saving/loading worlds
4. Survival/creative modes, health, hunger and crafting
5. Caves, ores and structures
6. Cross-platform multiplayer

This remains an early playable prototype, not a complete Minecraft 1.12.2 recreation. Collision is simplified, the world is small, and persistence, full crafting, mobs and multiplayer are not implemented. Windows builds are downloadable from GitHub Actions only after a successful workflow run.
