#pragma once
#include "Chunk.hpp"
#include <cstddef>
#include <iosfwd>
#include <unordered_map>
#include <string>
#include <vector>

namespace mc::world {
struct ChunkPos { int x{}; int z{}; bool operator==(const ChunkPos&) const = default; };
struct ChunkPosHash { std::size_t operator()(const ChunkPos& p) const noexcept; };
class World {
public:
    Chunk& get_or_create_chunk(int chunk_x,int chunk_z);
    const Chunk* find_chunk(int chunk_x,int chunk_z) const;
    BlockId get_block(int world_x,int y,int world_z) const;
    bool set_block(int world_x,int y,int world_z,BlockId block);
    std::size_t chunk_count() const { return chunks_.size(); }
    std::vector<ChunkPos> chunk_positions() const;
    bool unload_chunk(int chunk_x,int chunk_z);
    bool save_chunk_file(int chunk_x,int chunk_z,const std::string& directory) const;
    bool load_chunk_file(int chunk_x,int chunk_z,const std::string& directory);
    bool save(std::ostream& output) const;
    bool load(std::istream& input);
private:
    std::unordered_map<ChunkPos,Chunk,ChunkPosHash> chunks_;
};
}
