# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by the terrain and gameplay era of Minecraft Java 1.12.2. Independent implementation; no Minecraft source code or proprietary assets are included.

## Current milestone: terrain + multilingual foundation
- Deterministic seeded multi-octave terrain height field.
- Basic color bands for water, beaches, grasslands and highlands.
- Portable PPM terrain preview export.
- Built-in interface/message catalogs: English, Russian, Spanish, German, French and Simplified Chinese.
- Language selection with `--lang CODE` and English fallback for unsupported codes.
- Windows x64 build and CTest checks through GitHub Actions.

## Language examples
```powershell
.\build\Release\MineCppClone.exe --help --lang ru
.\build\Release\MineCppClone.exe --size 512 --seed 1122 --output terrain.ppm --lang en
.\build\Release\MineCppClone.exe --size 512 --seed 1122 --output terrain.ppm --lang zh_CN
```

Supported language codes: `en`, `ru`, `es`, `de`, `fr`, `zh_CN`.

## Build on Windows
Install Visual Studio 2022 Build Tools with the C++ workload and CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Roadmap
1. Terrain generation and world data
2. Localization expansion and in-game language menu
3. Chunk storage and block registry
4. 3D renderer and camera
5. Player movement and collision
6. Inventory, survival/creative modes
7. Save/load and cross-platform multiplayer

This is an early terrain prototype, not yet a complete playable game. The Windows executable is produced as a GitHub Actions artifact after a successful build.
