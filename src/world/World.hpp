#pragma once
#include "Chunk.hpp"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <unordered_map>
#include <vector>

namespace mc::world {
struct ChunkPos { int x{}; int z{}; bool operator==(const ChunkPos&) const = default; };
struct ChunkPosHash { std::size_t operator()(const ChunkPos& p) const noexcept; };
class World {
public:
    Chunk& get_or_create_chunk(int chunk_x,int chunk_z);
    const Chunk* find_chunk(int chunk_x,int chunk_z) const;
    bool erase_chunk(int chunk_x,int chunk_z);
    std::vector<ChunkPos> chunk_positions() const;
    bool save_chunk(std::ostream& output,int chunk_x,int chunk_z) const;
    bool load_chunk(std::istream& input,int chunk_x,int chunk_z);
    BlockId get_block(int world_x,int y,int world_z) const;
    bool set_block(int world_x,int y,int world_z,BlockId block);
    bool set_block_state(int world_x,int y,int world_z,const BlockState& state);
    const BlockState* get_block_state(int world_x,int y,int world_z) const;
    bool erase_block_state(int world_x,int y,int world_z);
    // Advances falling sand/gravel by one deterministic world tick.
    std::size_t tick_falling_blocks();
    // Bounded, deterministic water flow step; returns the number of changed blocks.
    std::size_t tick_water(std::size_t budget=96);
    std::uint64_t chunk_revision(int chunk_x,int chunk_z) const;
    std::size_t chunk_count() const { return chunks_.size(); }
    bool save(std::ostream& output) const;
    bool load(std::istream& input);
private:
    std::unordered_map<ChunkPos,Chunk,ChunkPosHash> chunks_;
};
}
