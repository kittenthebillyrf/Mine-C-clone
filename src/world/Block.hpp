#pragma once
#include <cstdint>
#include <string_view>

namespace mc::world {
enum class BlockId : std::uint16_t { Air=0, Stone=1, Dirt=2, Grass=3, Sand=4, Water=5, Bedrock=6 };
struct BlockInfo { BlockId id; std::string_view name; bool solid; };
const BlockInfo& block_info(BlockId id);
}
