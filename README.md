# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by Minecraft Java 1.12.2. Independent implementation; no Minecraft source code or proprietary assets are included.

## Current milestone: block-based 3D terrain
- Seeded 3D scene with block-column terrain rather than a smooth heightmap.
- Large terrain blocks with a 2x2x2 world-unit scale.
- Procedural hills, ridges, lowlands and basic climate-based surface regions.
- Water fill up to a shared sea level.
- Procedurally scattered simple trees.
- First-person camera: WASD + mouse; ESC closes the viewer.
- Block registry, 16 x 256 x 16 chunk storage and negative-coordinate world access.
- Six language catalogs: English, Russian, Spanish, German, French and Simplified Chinese.
- Windows x64 build and CTest workflow.

## Run
Open the 3D world:
```powershell
.\build\Release\MineCppClone.exe --3d --size 128 --seed 1122
```
Change the world size and seed:
```powershell
.\build\Release\MineCppClone.exe --3d --size 192 --seed 42
```
Export a 2D overview image:
```powershell
.\build\Release\MineCppClone.exe --size 512 --seed 1122 --output terrain.ppm --lang ru
```

## Build and test
Requires Visual Studio 2022 Build Tools with C++ workload and CMake. CMake fetches raylib 5.5 during configuration.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\MineCppClone.exe --self-test
```

## Roadmap
1. Block terrain generation and 3D preview
2. True voxel chunk meshing and chunk streaming
3. Player collision, jumping and block interaction
4. Caves, ores, rivers and richer biome rules
5. Inventory, survival/creative modes and crafting
6. Save/load and cross-platform multiplayer

Current limitations: terrain is rendered as large block-shaped columns, not yet as individually editable voxel blocks. Caves, proper chunk streaming and gameplay physics are not implemented yet. Windows builds are published as GitHub Actions artifacts after successful CI.
