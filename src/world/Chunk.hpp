#pragma once
#include "Block.hpp"
#include <array>
#include <bitset>
#include <cstdint>
#include <unordered_map>
#include <vector>

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
    // Fast access for validated local chunk coordinates (no bounds/hash lookup).
    BlockId get_unchecked(int x,int y,int z) const {
        return blocks_[std::size_t(y)*Width*Depth + std::size_t(z)*Width + std::size_t(x)];
    }
    bool set(int x,int y,int z,BlockId block);
    std::size_t count(BlockId block) const;
    bool set_state(int x,int y,int z,const BlockState& state);
    const BlockState* get_state(int x,int y,int z) const;
    bool erase_state(int x,int y,int z);
    std::uint64_t revision() const { return revision_; }
    std::size_t state_count() const { return states_.size(); }
    const std::unordered_map<std::size_t,BlockState>& state_entries() const { return states_; }
    const std::vector<std::size_t>& falling_blocks() const { return falling_blocks_; }
    // Unique positions of non-full-cube blocks; stale entries are filtered on read.
    const std::vector<std::size_t>& special_blocks() const { return special_blocks_; }
private:
    static std::size_t index(int x,int y,int z);
    std::array<BlockId,Volume> blocks_{};
    std::unordered_map<std::size_t,BlockState> states_;
    std::vector<std::size_t> falling_blocks_;
    std::vector<std::size_t> special_blocks_;
    std::bitset<Volume> special_flags_;
    std::uint64_t revision_=0;
};
}
