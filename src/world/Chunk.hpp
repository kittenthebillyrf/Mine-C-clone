#pragma once
#include "Block.hpp"
#include <array>
#include <cstdint>

namespace mc::world {
class Chunk {
public:
    static constexpr int Width=16;
    static constexpr int Height=256;
    static constexpr int Depth=16;
    static constexpr std::size_t Volume=std::size_t(Width)*Height*Depth;
    Chunk();
    BlockId get(int x,int y,int z) const;
    bool set(int x,int y,int z,BlockId block);
    std::size_t count(BlockId block) const;
private:
    static std::size_t index(int x,int y,int z);
    std::array<BlockId,Volume> blocks_{};
};
}
