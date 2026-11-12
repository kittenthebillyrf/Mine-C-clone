#include "World.hpp"
#include <cstdint>

namespace mc::world {
std::size_t ChunkPosHash::operator()(const ChunkPos& p) const noexcept {
    const auto x=static_cast<std::uint32_t>(p.x);
    const auto z=static_cast<std::uint32_t>(p.z);
    return (std::size_t(x)*0x9e3779b1u) ^ (std::size_t(z)+0x85ebca6bu+(std::size_t(x)<<6)+(std::size_t(x)>>2));
}
namespace {
int floor_div16(int value) { return value>=0 ? value/16 : -((15-value)/16); }
int local16(int value) { const int r=value%16; return r<0?r+16:r; }
}
Chunk& World::get_or_create_chunk(int chunk_x,int chunk_z) { return chunks_[ChunkPos{chunk_x,chunk_z}]; }
const Chunk* World::find_chunk(int chunk_x,int chunk_z) const {
    const auto it=chunks_.find(ChunkPos{chunk_x,chunk_z});
    return it==chunks_.end()?nullptr:&it->second;
}
BlockId World::get_block(int wx,int y,int wz) const {
    if(y<0||y>=Chunk::Height) return BlockId::Air;
    const auto* chunk=find_chunk(floor_div16(wx),floor_div16(wz));
    return chunk?chunk->get(local16(wx),y,local16(wz)):BlockId::Air;
}
bool World::set_block(int wx,int y,int wz,BlockId block) {
    if(y<0||y>=Chunk::Height) return false;
    return get_or_create_chunk(floor_div16(wx),floor_div16(wz)).set(local16(wx),y,local16(wz),block);
}
}
