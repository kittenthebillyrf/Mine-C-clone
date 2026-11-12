# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by the terrain and gameplay era of Minecraft Java 1.12.2. Independent implementation; no Minecraft source code or proprietary assets are included.

## Current milestone: terrain + voxel world core
- Deterministic seeded multi-octave terrain height field with PPM preview export.
- Six built-in interface languages: English, Russian, Spanish, German, French and Simplified Chinese.
- Block registry with IDs and basic solidity metadata.
- 16 x 256 x 16 voxel chunk storage.
- World chunk lookup and block access, including negative world coordinates.
- CMake build and GitHub Actions Windows x64 workflow.

## Language examples
```powershell
.\build\Release\MineCppClone.exe --help --lang ru
.\build\Release\MineCppClone.exe --size 512 --seed 1122 --output terrain.ppm --lang en
```
Language codes: `en`, `ru`, `es`, `de`, `fr`, `zh_CN`.

## Build and test on Windows
Install Visual Studio 2022 Build Tools with the C++ workload and CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\MineCppClone.exe --self-test
```

## Roadmap
1. Terrain generation and world data
2. Chunk storage and block registry
3. 3D renderer, mesh generation and camera
4. Player movement and collision
5. Inventory, survival/creative modes and crafting
6. Save/load and cross-platform multiplayer

This remains an early engine prototype. It does not yet render a playable 3D world. The Windows executable is available as a GitHub Actions artifact only after a successful workflow run.
