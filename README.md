# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by the terrain and gameplay era of Minecraft Java 1.12.2. Independent implementation; no Minecraft source code or proprietary assets are included.

## Current milestone: procedural terrain
- Deterministic seeded multi-octave terrain height field.
- Basic color bands for water, beaches, grasslands and highlands.
- Exports a portable PPM terrain preview.
- Windows x64 build and test workflow through GitHub Actions.

## Build on Windows
Install Visual Studio 2022 Build Tools with the C++ workload and CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\MineCppClone.exe --size 512 --seed 1122 --output terrain.ppm
```

## Roadmap
1. Terrain generation and world data
2. Chunk storage and block registry
3. 3D renderer and camera
4. Player movement and collision
5. Inventory, survival/creative modes
6. Save/load and cross-platform multiplayer

This is an early terrain prototype, not yet a complete playable game. The Windows executable is produced as a GitHub Actions artifact after a successful build.
