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
    CraftingTable=33, Furnace=34, OakSlab=35, OakFence=36, Lava=37,
    SprucePlanks=38, BirchPlanks=39, JunglePlanks=40, AcaciaPlanks=41, DarkOakPlanks=42,
    SpruceLog=43, BirchLog=44, JungleLog=45, AcaciaLog=46, DarkOakLog=47,
    SpruceLeaves=48, BirchLeaves=49, JungleLeaves=50, AcaciaLeaves=51, DarkOakLeaves=52,
    Granite=53, PolishedGranite=54, Diorite=55, PolishedDiorite=56, Andesite=57, PolishedAndesite=58,
    StoneBricks=59, MossyStoneBricks=60, CrackedStoneBricks=61, ChiseledStoneBricks=62,
    Sponge=63, WetSponge=64, Prismarine=65, PrismarineBricks=66, DarkPrismarine=67,
    SeaLantern=68, EndStone=69, EndBricks=70, PurpurBlock=71, PurpurPillar=72,
    QuartzBlock=73, ChiseledQuartz=74, QuartzPillar=75, NetherQuartzOre=76,
    RedstoneLamp=77, HayBale=78, Melon=79, PumpkinLit=80, Terracotta=81,
    WhiteConcrete=82, OrangeConcrete=83, MagentaConcrete=84, LightBlueConcrete=85,
    YellowConcrete=86, LimeConcrete=87, PinkConcrete=88, GrayConcrete=89,
    LightGrayConcrete=90, CyanConcrete=91, PurpleConcrete=92, BlueConcrete=93,
    BrownConcrete=94, GreenConcrete=95, RedConcrete=96, BlackConcrete=97,
    Torch=98, OakStairs=99, Count=100
};
inline constexpr std::uint16_t BLOCK_ID_COUNT=static_cast<std::uint16_t>(BlockId::Count);
struct BlockInfo { BlockId id; std::string_view name; bool solid; };
const BlockInfo& block_info(BlockId id);
}
