#include "Block.hpp"
#include <array>

namespace mc::world {
const BlockInfo& block_info(BlockId id) {
    static constexpr std::array<BlockInfo,BLOCK_ID_COUNT> blocks{{
        {BlockId::Air,"air",false},
        {BlockId::Stone,"stone",true},
        {BlockId::Dirt,"dirt",true},
        {BlockId::Grass,"grass_block",true},
        {BlockId::Sand,"sand",true},
        {BlockId::Water,"water",false},
        {BlockId::Bedrock,"bedrock",true},
        {BlockId::Cobblestone,"cobblestone",true},
        {BlockId::OakPlanks,"oak_planks",true},
        {BlockId::OakLog,"oak_log",true},
        {BlockId::OakLeaves,"oak_leaves",true},
        {BlockId::Gravel,"gravel",true},
        {BlockId::Sandstone,"sandstone",true},
        {BlockId::Glass,"glass",true},
        {BlockId::Bricks,"bricks",true},
        {BlockId::Clay,"clay",true},
        {BlockId::CoalOre,"coal_ore",true},
        {BlockId::IronOre,"iron_ore",true},
        {BlockId::GoldOre,"gold_ore",true},
        {BlockId::DiamondOre,"diamond_ore",true},
        {BlockId::RedstoneOre,"redstone_ore",true},
        {BlockId::LapisOre,"lapis_ore",true},
        {BlockId::EmeraldOre,"emerald_ore",true},
        {BlockId::Obsidian,"obsidian",true},
        {BlockId::Snow,"snow",true},
        {BlockId::Ice,"ice",true},
        {BlockId::Cactus,"cactus",true},
        {BlockId::Bookshelf,"bookshelf",true},
        {BlockId::WoolWhite,"white_wool",true},
        {BlockId::Netherrack,"netherrack",true},
        {BlockId::Glowstone,"glowstone",true},
        {BlockId::MossyCobblestone,"mossy_cobblestone",true},
        {BlockId::Pumpkin,"pumpkin",true},
        {BlockId::CraftingTable,"crafting_table",true},
        {BlockId::Furnace,"furnace",true},
        {BlockId::OakSlab,"oak_slab",true},
        {BlockId::OakFence,"oak_fence",true}
    }};
    const auto raw=static_cast<std::uint16_t>(id);
    return blocks[raw<blocks.size()?raw:0];
}
}
