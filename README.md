# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by Minecraft Java 1.12.2. This is an independent implementation and includes no Minecraft source code or proprietary assets.

## Run

```powershell
.\build\Release\MineCppClone.exe --3d --size 64 --seed 1122
```

## Controls

- WASD — move; mouse — look
- Space — jump; Left Shift — run
- Left mouse — break the targeted block
- Right mouse — place a block from the selected stack
- 1–9 / mouse wheel — select hotbar slot
- E — open/close inventory
- In inventory, click a hotbar slot to select it; click a storage slot to swap it with the selected hotbar slot
- Escape — release / recapture the mouse

## Implemented

- First-person camera, walking, running and jumping.
- Seeded terrain with grass, dirt, stone, sand, bedrock and water.
- Block targeting, breaking and placement.
- 9-slot hotbar, 36-slot inventory, stack limit of 64.
- Versioned binary world save format; the game loads `world.mcc` on startup and saves it when the window closes.
- 16 x 256 x 16 chunk storage and support for negative world coordinates.
- Six language catalogs: English, Russian, Spanish, German, French and Simplified Chinese.

## Optional map export

```powershell
.\build\Release\MineCppClone.exe --map --size 128 --seed 1122 --output terrain.ppm
```

## Build and tests

Requires Visual Studio 2022 Build Tools with the C++ workload and CMake. CMake downloads raylib 5.5 during configuration.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\MineCppClone.exe --self-test
```

## Next milestones

1. Reliable voxel collision and block-face targeting
2. Chunk mesh generation and distance-based chunk streaming
3. Survival/creative modes, health, hunger and crafting
4. Caves, ores and structures
5. Cross-platform multiplayer

This is an early playable prototype, not a complete Minecraft 1.12.2 recreation. Collision is still simplified; full crafting, mobs and multiplayer are not implemented.
