## Crafting update
The survival inventory now includes a working 2x2 crafting grid with these recipes:
- 1 oak log -> 4 planks
- 2 vertically aligned planks -> 4 sticks
- 4 planks -> 1 crafting table

Click a hotbar stack to select it, click a crafting slot to add one item, then click the output slot to craft. Crafted items can be moved to the hotbar and placed where applicable.

# Mine C++ Clone

PC-first C++20 voxel sandbox prototype inspired by Minecraft Java 1.12.2. This is an independent implementation; it does not bundle Minecraft source code or proprietary assets.

## Run

```powershell
.\build\Release\MineCppClone.exe --3d --size 4096 --seed 1122
```

The playable world is bounded to a 4096 x 4096 block area. Chunks are generated on demand around the player; the complete world is not generated at startup.

## World generation

- Deterministic, seed-based multi-octave terrain fields.
- Separate continentalness, erosion, ridge, detail, temperature and rainfall signals.
- Plains, forest, desert, taiga, tundra, swamp, savanna, rainforest, seasonal forest, shrubland and ice-desert biome classification.
- River-like lowland channels, sea-level water, sand and gravel surface patches.
- Layered bedrock, stone, dirt and biome-aware surface materials.
- Interpolated 3D cave fields, plus coal and iron ore deposits.
- Seeded tree placement with trunks and leaf canopies.
- Shared generation code for the playable world and exported height/biome map.

The generator follows broad architectural ideas visible in the supplied Pocket Edition sources (seeded octave noise, biome climate fields, density/surface passes and feature decoration), with an independently written C++ implementation and Java-era terrain shaping ideas. It is not a byte-for-byte port of Minecraft source.

## Controls

### Keyboard and mouse
- WASD — move; mouse — look
- Space — jump; Left Shift — crouch
- Left mouse — break the targeted block
- Right mouse — place a block from the selected stack
- 1–9 / mouse wheel — select hotbar slot
- E — inventory; C — crafting panel; R — creative panel
- Escape — pause / release mouse

### Nintendo Switch
- Left stick — move; right stick — look
- A — jump / confirm
- B — back / cancel
- X — inventory
- Y — crafting panel
- Plus — pause menu
- D-pad — navigate menus; L/R — change hotbar slot
- ZR — break; ZL — place
- HOME Menu icon is generated from `assets/switch/icon.svg` during the NRO workflow.

The game detects its compile-time target platform and offers a recommended performance profile on startup. Render distance is configured in chunks in Pause → Settings and saved to `settings.cfg`. The recommendation is a starting point; real hardware performance still needs to be measured.

## Implemented prototype systems

- First-person camera, basic walking, jumping and block interaction.
- 9-slot hotbar, 36-slot inventory and stack limit of 64.
- 16 x 256 x 16 block chunks and negative-coordinate support.
- Versioned binary world save/load.
- English, Russian, Spanish, German, French and Simplified Chinese catalogs.
- 4096-block world bounds and on-demand chunk generation.

## Map export

```powershell
.\build\Release\MineCppClone.exe --map --size 4096 --seed 1122 --output terrain.ppm
```

## Build and tests

Requires Visual Studio 2022 Build Tools with the C++ workload and CMake. CMake downloads raylib 5.5 during configuration.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\MineCppClone.exe --self-test
```

A GitHub Actions workflow also configures, builds and runs CTest on the development branch.

## Remaining work

The project is still an early playable prototype, not a complete Minecraft 1.12.2 recreation. Native build/runtime verification is pending. Chunk mesh rendering, robust collision, complete inventory/crafting, mobs, and multiplayer are not implemented.
