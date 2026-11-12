#include "Block.hpp"
#include <array>

namespace mc::world {
const BlockInfo& block_info(BlockId id) {
    static constexpr std::array<BlockInfo,7> blocks{{
        {BlockId::Air,"air",false},
        {BlockId::Stone,"stone",true},
        {BlockId::Dirt,"dirt",true},
        {BlockId::Grass,"grass_block",true},
        {BlockId::Sand,"sand",true},
        {BlockId::Water,"water",false},
        {BlockId::Bedrock,"bedrock",true}
    }};
    const auto raw=static_cast<std::uint16_t>(id);
    return blocks[raw<blocks.size()?raw:0];
}
}
