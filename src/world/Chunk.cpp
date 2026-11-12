#include "Chunk.hpp"
#include <algorithm>

namespace mc::world {
Chunk::Chunk() { blocks_.fill(BlockId::Air); }
std::size_t Chunk::index(int x,int y,int z) {
    return std::size_t(y)*Width*Depth + std::size_t(z)*Width + std::size_t(x);
}
BlockId Chunk::get(int x,int y,int z) const {
    if(x<0||x>=Width||y<0||y>=Height||z<0||z>=Depth) return BlockId::Air;
    return blocks_[index(x,y,z)];
}
bool Chunk::set(int x,int y,int z,BlockId block) {
    if(x<0||x>=Width||y<0||y>=Height||z<0||z>=Depth) return false;
    blocks_[index(x,y,z)]=block;
    return true;
}
std::size_t Chunk::count(BlockId block) const {
    return std::size_t(std::count(blocks_.begin(),blocks_.end(),block));
}
}
