#pragma once
#include "Block.hpp"
#include <array>
#include <cstdint>
#include <unordered_map>

namespace mc::world {
struct BlockState {
    std::uint8_t facing=0;
    std::uint8_t shape=0;
    std::uint8_t power=0;
    std::uint8_t flags=0;
};
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
    bool set_state(int x,int y,int z,const BlockState& state);
    const BlockState* get_state(int x,int y,int z) const;
    bool erase_state(int x,int y,int z);
    std::uint64_t revision() const { return revision_; }
    std::size_t state_count() const { return states_.size(); }
    const std::unordered_map<std::size_t,BlockState>& state_entries() const { return states_; }
private:
    static std::size_t index(int x,int y,int z);
    std::array<BlockId,Volume> blocks_{};
    std::unordered_map<std::size_t,BlockState> states_;
    std::uint64_t revision_=0;
};
}
