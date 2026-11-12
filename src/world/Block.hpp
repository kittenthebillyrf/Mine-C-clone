#pragma once
#include <cstdint>
#include <string_view>

namespace mc::world {
// Stable numeric IDs: saved worlds keep their block identities across builds.
enum class BlockId : std::uint16_t {
    Air=0, Stone=1, Dirt=2, Grass=3, Sand=4, Water=5, Bedrock=6,
    Cobblestone=7, OakPlanks=8, OakLog=9, OakLeaves=10, Gravel=11,
    Sandstone=12, Glass=13, Bricks=14, Clay=15, CoalOre=16, IronOre=17,
    GoldOre=18, DiamondOre=19, RedstoneOre=20, LapisOre=21, EmeraldOre=22,
    Obsidian=23, Snow=24, Ice=25, Cactus=26, Bookshelf=27, WoolWhite=28,
    Netherrack=29, Glowstone=30, MossyCobblestone=31, Pumpkin=32,
    CraftingTable=33, Furnace=34, OakSlab=35, OakFence=36,
    Count=37
};
inline constexpr std::uint16_t BLOCK_ID_COUNT=static_cast<std::uint16_t>(BlockId::Count);
struct BlockInfo { BlockId id; std::string_view name; bool solid; };
const BlockInfo& block_info(BlockId id);
}
