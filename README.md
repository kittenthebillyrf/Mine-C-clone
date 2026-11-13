# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by Minecraft Java 1.12.2. This is an independent implementation and includes no Minecraft source code or proprietary assets.

## Run

```powershell
.\build\Release\MineCppClone.exe --3d --size 64 --seed 1122
```

## Main menu

The game opens on a Java 1.12-era inspired main menu with a slowly rotating 3D voxel panorama, centered title, and gray buttons that darken on hover. Singleplayer enters the world; Multiplayer is marked as a future feature; Options currently contains a placeholder panel; Quit Game closes the application.

## Controls

- WASD — move; mouse — look
- Space — jump; Left Control — sprint; Left Shift — crouch
- Left mouse — break the targeted block
- Right mouse — place a block from the selected stack
- 1–9 / mouse wheel — select hotbar slot
- E — open/close inventory
- Escape — open/close the pause menu
- Pause menu: Up/Down + Enter, or D-pad + A on a gamepad
- Pause actions: Continue, Save World, Save and Quit
- In inventory, click a hotbar slot to select it; click a storage slot to swap it with the selected hotbar slot

## Implemented

- First-person camera, walking, running and jumping.
- Cached 16×16×16 chunk-section meshes for full cube terrain, greedy face merging, atlas UVs and neighbor-revision invalidation; partial/transparent blocks remain on the model path.
- Seeded terrain with grass, dirt, stone, sand, bedrock and water.
- Block targeting, breaking and placement.
- 9-slot hotbar, 36-slot inventory, stack limit of 64.
- Independent, versioned `world.mcc` chunk/block save format (not Java Anvil/NBT and no `.mca` import/export).
- Player session sidecar `world.mcc.player`: position, velocity, camera direction, hotbar selection and all 36 inventory slots.
- Pause-menu save and save-and-quit, plus save on normal window close; temporary files and backup replacement protect existing saves.
- Save loading validates the player position and moves the player to a free position if terrain now occupies the saved hitbox.
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
2. Distance-based chunk streaming, atlas-safe repeated UVs and partial-block mesh batching
3. Survival/creative modes, health, hunger and crafting
4. Caves, ores and structures
5. Cross-platform multiplayer

This is an early playable prototype, not a complete Minecraft 1.12.2 recreation. Player collision uses a swept, axis-separated AABB solver with partial shapes for slabs, stairs and fences, but still needs in-game regression testing. Full crafting, mobs and multiplayer are not implemented. Chunk meshing is now connected to the runtime renderer for full cubes; atlas textures on merged faces are currently stretched across merged quads, and partial/transparent geometry still uses the individual model path.
