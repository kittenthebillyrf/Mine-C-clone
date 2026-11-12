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
    const auto i=index(x,y,z);
    if(blocks_[i]!=block) {
        blocks_[i]=block;
        if(block==BlockId::Air) states_.erase(i);
        ++revision_;
    }
    return true;
}
std::size_t Chunk::count(BlockId block) const {
    return std::size_t(std::count(blocks_.begin(),blocks_.end(),block));
}
bool Chunk::set_state(int x,int y,int z,const BlockState& state) {
    if(x<0||x>=Width||y<0||y>=Height||z<0||z>=Depth) return false;
    if(get(x,y,z)==BlockId::Air) return false;
    states_[index(x,y,z)]=state;
    ++revision_;
    return true;
}
const BlockState* Chunk::get_state(int x,int y,int z) const {
    if(x<0||x>=Width||y<0||y>=Height||z<0||z>=Depth) return nullptr;
    const auto it=states_.find(index(x,y,z));
    return it==states_.end()?nullptr:&it->second;
}
bool Chunk::erase_state(int x,int y,int z) {
    if(x<0||x>=Width||y<0||y>=Height||z<0||z>=Depth) return false;
    if(states_.erase(index(x,y,z))==0) return false;
    ++revision_;
    return true;
}
}
