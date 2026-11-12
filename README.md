# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by the terrain and gameplay era of Minecraft Java 1.12.2. Independent implementation; no Minecraft source code or proprietary assets are included.

## Current milestone: first 3D terrain preview
- Seeded procedural heightmap generation.
- Interactive 3D terrain viewer powered by raylib 5.5.
- First-person camera controls: WASD + mouse; ESC closes the viewer.
- Block registry and 16 x 256 x 16 chunk storage.
- World block access works across positive and negative chunk coordinates.
- Six language catalogs: English, Russian, Spanish, German, French and Simplified Chinese.
- Windows x64 build and CTest workflow.

## Run
Open the 3D preview:
```powershell
.\build\Release\MineCppClone.exe --3d --size 256 --seed 1122
```
Export a terrain image:
```powershell
.\build\Release\MineCppClone.exe --size 512 --seed 1122 --output terrain.ppm --lang ru
```
Show translated help:
```powershell
.\build\Release\MineCppClone.exe --help --lang ru
```
Language codes: `en`, `ru`, `es`, `de`, `fr`, `zh_CN`.

## Build and test
Requires Visual Studio 2022 Build Tools with C++ workload and CMake. CMake fetches raylib 5.5 during configuration.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\MineCppClone.exe --self-test
```

## Roadmap
1. Terrain and world data
2. Chunk storage and block registry
3. 3D rendering and camera prototype
4. Player collision and voxel interaction
5. Inventory, survival/creative modes and crafting
6. Save/load and cross-platform multiplayer

The 3D preview currently renders a generated heightmap as a terrain surface; it is not yet a fully block-by-block Minecraft-style world or a complete playable game. Windows builds are published as GitHub Actions artifacts after successful CI.
