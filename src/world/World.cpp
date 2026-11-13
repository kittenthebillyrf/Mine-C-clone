#include "World.hpp"
#include <algorithm>
#include <cstdint>
#include <istream>
#include <ostream>
#include <unordered_map>
#include <utility>

namespace mc::world {
std::size_t ChunkPosHash::operator()(const ChunkPos& p) const noexcept {
    const auto x=static_cast<std::uint32_t>(p.x);
    const auto z=static_cast<std::uint32_t>(p.z);
    return (std::size_t(x)*0x9e3779b1u) ^ (std::size_t(z)+0x85ebca6bu+(std::size_t(x)<<6)+(std::size_t(x)>>2));
}
namespace {
int floor_div16(int value) { return value>=0 ? value/16 : -((15-value)/16); }
int local16(int value) { const int r=value%16; return r<0?r+16:r; }
bool write_u32(std::ostream& out,std::uint32_t v) {
    for(int i=0;i<4;++i) out.put(static_cast<char>((v>>(i*8))&0xffu));
    return bool(out);
}
bool read_u32(std::istream& in,std::uint32_t& v) {
    v=0;
    for(int i=0;i<4;++i) {
        const int c=in.get();
        if(c==std::char_traits<char>::eof()) return false;
        v|=std::uint32_t(static_cast<unsigned char>(c))<<(i*8);
    }
    return true;
}
}
Chunk& World::get_or_create_chunk(int chunk_x,int chunk_z) { return chunks_[ChunkPos{chunk_x,chunk_z}]; }
const Chunk* World::find_chunk(int chunk_x,int chunk_z) const {
    const auto it=chunks_.find(ChunkPos{chunk_x,chunk_z});
    return it==chunks_.end()?nullptr:&it->second;
}
bool World::erase_chunk(int chunk_x,int chunk_z) {
    return chunks_.erase(ChunkPos{chunk_x,chunk_z})!=0;
}
std::vector<ChunkPos> World::chunk_positions() const {
    std::vector<ChunkPos> result;
    result.reserve(chunks_.size());
    for(const auto& entry:chunks_) result.push_back(entry.first);
    return result;
}
bool World::save_chunk(std::ostream& output,int cx,int cz) const {
    const auto* chunk=find_chunk(cx,cz);
    if(!chunk) return false;
    static constexpr char magic[8]={'M','C','C','C','H','N','K','1'};
    output.write(magic,sizeof(magic));
    if(!output||!write_u32(output,1u)||!write_u32(output,static_cast<std::uint32_t>(cx))||
       !write_u32(output,static_cast<std::uint32_t>(cz))) return false;
    for(int y=0;y<Chunk::Height;++y) for(int z=0;z<Chunk::Depth;++z) for(int x=0;x<Chunk::Width;++x) {
        const auto id=static_cast<std::uint16_t>(chunk->get(x,y,z));
        output.put(static_cast<char>(id&0xffu));
        output.put(static_cast<char>((id>>8)&0xffu));
        if(!output) return false;
    }
    if(!write_u32(output,static_cast<std::uint32_t>(chunk->state_count()))) return false;
    for(const auto& entry:chunk->state_entries()) {
        if(!write_u32(output,static_cast<std::uint32_t>(entry.first))) return false;
        output.put(static_cast<char>(entry.second.facing));
        output.put(static_cast<char>(entry.second.shape));
        output.put(static_cast<char>(entry.second.power));
        output.put(static_cast<char>(entry.second.flags));
        if(!output) return false;
    }
    return true;
}
bool World::load_chunk(std::istream& input,int cx,int cz) {
    static constexpr char magic[8]={'M','C','C','C','H','N','K','1'};
    char header[8]{};
    input.read(header,sizeof(header));
    if(!input||!std::equal(std::begin(header),std::end(header),std::begin(magic))) return false;
    std::uint32_t version=0,rawX=0,rawZ=0;
    if(!read_u32(input,version)||version!=1u||!read_u32(input,rawX)||!read_u32(input,rawZ)||
       static_cast<std::int32_t>(rawX)!=cx||static_cast<std::int32_t>(rawZ)!=cz) return false;
    Chunk loaded;
    for(int y=0;y<Chunk::Height;++y) for(int z=0;z<Chunk::Depth;++z) for(int x=0;x<Chunk::Width;++x) {
        const int lo=input.get(),hi=input.get();
        if(lo==std::char_traits<char>::eof()||hi==std::char_traits<char>::eof()) return false;
        const std::uint16_t id=static_cast<std::uint16_t>(static_cast<unsigned char>(lo))|
                               static_cast<std::uint16_t>(static_cast<unsigned char>(hi)<<8);
        if(id>=BLOCK_ID_COUNT||!loaded.set(x,y,z,static_cast<BlockId>(id))) return false;
    }
    std::uint32_t stateCount=0;
    if(!read_u32(input,stateCount)||stateCount>Chunk::Volume) return false;
    for(std::uint32_t i=0;i<stateCount;++i) {
        std::uint32_t index=0;
        if(!read_u32(input,index)||index>=Chunk::Volume) return false;
        const int x=int(index%Chunk::Width),z=int((index/Chunk::Width)%Chunk::Depth);
        const int y=int(index/(Chunk::Width*Chunk::Depth));
        const int facing=input.get(),shape=input.get(),power=input.get(),flags=input.get();
        if(facing<0||shape<0||power<0||flags<0||loaded.get(x,y,z)==BlockId::Air||
           loaded.get_state(x,y,z)!=nullptr) return false;
        if(!loaded.set_state(x,y,z,BlockState{static_cast<std::uint8_t>(facing),
            static_cast<std::uint8_t>(shape),static_cast<std::uint8_t>(power),static_cast<std::uint8_t>(flags)})) return false;
    }
    if(input.peek()!=std::char_traits<char>::eof()) return false;
    chunks_[ChunkPos{cx,cz}]=std::move(loaded);
    return true;
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
bool World::set_block_state(int wx,int y,int wz,const BlockState& state) {
    if(y<0||y>=Chunk::Height) return false;
    return get_or_create_chunk(floor_div16(wx),floor_div16(wz)).set_state(local16(wx),y,local16(wz),state);
}
const BlockState* World::get_block_state(int wx,int y,int wz) const {
    if(y<0||y>=Chunk::Height) return nullptr;
    const auto* chunk=find_chunk(floor_div16(wx),floor_div16(wz));
    return chunk?chunk->get_state(local16(wx),y,local16(wz)):nullptr;
}
bool World::erase_block_state(int wx,int y,int wz) {
    if(y<0||y>=Chunk::Height) return false;
    auto it=chunks_.find(ChunkPos{floor_div16(wx),floor_div16(wz)});
    return it!=chunks_.end()&&it->second.erase_state(local16(wx),y,local16(wz));
}
std::uint64_t World::chunk_revision(int cx,int cz) const {
    const auto* chunk=find_chunk(cx,cz);
    return chunk?chunk->revision():0;
}
std::size_t World::tick_falling_blocks() {
    std::size_t moved=0;
    // Iterate bottom-up so a falling block can move only one cell per tick.
    // A column of sand/gravel therefore descends as a stack instead of
    // teleporting all the way down during one update.
    for(auto& entry:chunks_) {
        Chunk& chunk=entry.second;
        auto falling=chunk.falling_blocks();
        std::sort(falling.begin(),falling.end());
        for(const std::size_t index:falling) {
            const int x=int(index%Chunk::Width);
            const int z=int((index/Chunk::Width)%Chunk::Depth);
            const int y=int(index/(Chunk::Width*Chunk::Depth));
            if(y<=0) continue;
            const BlockId id=chunk.get(x,y,z);
            if(id!=BlockId::Sand&&id!=BlockId::Gravel) continue;
            const BlockId below=chunk.get(x,y-1,z);
            const bool replaceable=below==BlockId::Air||below==BlockId::Water||below==BlockId::Lava;
            if(!replaceable) continue;
            chunk.set(x,y,z,below);
            chunk.set(x,y-1,z,id);
            ++moved;
        }
    }
    return moved;
}

std::size_t World::tick_water(std::size_t budget) {
    if(budget==0) return 0;
    struct WaterCell { int x,y,z; std::uint8_t level; bool source; };
    std::vector<WaterCell> active;
    // Snapshot the current water set: newly flowed cells are processed on a
    // later tick, keeping the work bounded and independent of hash iteration.
    for(const auto& entry:chunks_) {
        const int ox=entry.first.x*Chunk::Width,oz=entry.first.z*Chunk::Depth;
        const Chunk& chunk=entry.second;
        for(const std::size_t index:chunk.special_blocks()) {
            const int lx=int(index%Chunk::Width);
            const int lz=int((index/Chunk::Width)%Chunk::Depth);
            const int y=int(index/(Chunk::Width*Chunk::Depth));
            if(chunk.get_unchecked(lx,y,lz)!=BlockId::Water) continue;
            const BlockState* state=chunk.get_state(lx,y,lz);
            const bool flowing=state&&state->flags==1;
            active.push_back({ox+lx,y,oz+lz,static_cast<std::uint8_t>(flowing?std::clamp<int>(state->power,1,7):0),!flowing});
        }
    }
    std::size_t changed=0;
    for(const WaterCell& cell:active) {
        if(changed>=budget) break;
        if(get_block(cell.x,cell.y,cell.z)!=BlockId::Water) continue;
        const auto flow=[&](int x,int y,int z,std::uint8_t level) {
            if(changed>=budget||y<0||y>=Chunk::Height) return;
            const BlockId target=get_block(x,y,z);
            if(target!=BlockId::Air) return;
            if(set_block(x,y,z,BlockId::Water)) {
                set_block_state(x,y,z,BlockState{0,0,level,1});
                ++changed;
            }
        };
        // Vanilla-style priority: downward flow first, then four horizontal
        // directions with increasing decay level. Source blocks remain stable.
        if(cell.y>0 && get_block(cell.x,cell.y-1,cell.z)==BlockId::Air) {
            flow(cell.x,cell.y-1,cell.z,1);
            continue;
        }
        const std::uint8_t next=static_cast<std::uint8_t>(cell.source?1:cell.level+1);
        if(next>7) continue;
        flow(cell.x+1,cell.y,cell.z,next);
        flow(cell.x-1,cell.y,cell.z,next);
        flow(cell.x,cell.y,cell.z+1,next);
        flow(cell.x,cell.y,cell.z-1,next);
    }
    return changed;
}
bool World::save(std::ostream& output) const {
    static constexpr char magic[8]={'M','C','C','W','O','R','L','D'};
    output.write(magic,sizeof(magic));
    if(!output||!write_u32(output,2u)||!write_u32(output,static_cast<std::uint32_t>(chunks_.size()))) return false;
    for(const auto& entry:chunks_) {
        if(!write_u32(output,static_cast<std::uint32_t>(entry.first.x))||
           !write_u32(output,static_cast<std::uint32_t>(entry.first.z))) return false;
        const Chunk& chunk=entry.second;
        for(int y=0;y<Chunk::Height;++y) for(int z=0;z<Chunk::Depth;++z) for(int x=0;x<Chunk::Width;++x) {
            const auto id=static_cast<std::uint16_t>(chunk.get(x,y,z));
            output.put(static_cast<char>(id&0xffu));
            output.put(static_cast<char>((id>>8)&0xffu));
            if(!output) return false;
        }
        if(!write_u32(output,static_cast<std::uint32_t>(chunk.state_count()))) return false;
        for(const auto& stateEntry:chunk.state_entries()) {
            const auto index=static_cast<std::uint32_t>(stateEntry.first);
            if(!write_u32(output,index)) return false;
            output.put(static_cast<char>(stateEntry.second.facing));
            output.put(static_cast<char>(stateEntry.second.shape));
            output.put(static_cast<char>(stateEntry.second.power));
            output.put(static_cast<char>(stateEntry.second.flags));
            if(!output) return false;
        }
    }
    return true;
}
bool World::load(std::istream& input) {
    static constexpr char magic[8]={'M','C','C','W','O','R','L','D'};
    char header[8]{};
    input.read(header,sizeof(header));
    if(!input||!std::equal(std::begin(header),std::end(header),std::begin(magic))) return false;
    std::uint32_t version=0,count=0;
    if(!read_u32(input,version)||(version!=1u&&version!=2u)||!read_u32(input,count)||count>4096u) return false;
    std::unordered_map<ChunkPos,Chunk,ChunkPosHash> loaded;
    for(std::uint32_t n=0;n<count;++n) {
        std::uint32_t rawX=0,rawZ=0;
        if(!read_u32(input,rawX)||!read_u32(input,rawZ)) return false;
        const ChunkPos pos{static_cast<std::int32_t>(rawX),static_cast<std::int32_t>(rawZ)};
        auto inserted=loaded.try_emplace(pos);
        if(!inserted.second) return false;
        Chunk& chunk=inserted.first->second;
        for(int y=0;y<Chunk::Height;++y) for(int z=0;z<Chunk::Depth;++z) for(int x=0;x<Chunk::Width;++x) {
            const int lo=input.get(),hi=input.get();
            if(lo==std::char_traits<char>::eof()||hi==std::char_traits<char>::eof()) return false;
            const std::uint16_t id=static_cast<std::uint16_t>(static_cast<unsigned char>(lo))|
                                   static_cast<std::uint16_t>(static_cast<unsigned char>(hi)<<8);
            if(id>=BLOCK_ID_COUNT) return false;
            if(!chunk.set(x,y,z,static_cast<BlockId>(id))) return false;
        }
        if(version>=2u) {
            std::uint32_t stateCount=0;
            if(!read_u32(input,stateCount)||stateCount>Chunk::Volume) return false;
            for(std::uint32_t i=0;i<stateCount;++i) {
                std::uint32_t index=0;
                if(!read_u32(input,index)||index>=Chunk::Volume) return false;
                const int x=int(index%Chunk::Width);
                const int z=int((index/Chunk::Width)%Chunk::Depth);
                const int y=int(index/(Chunk::Width*Chunk::Depth));
                const int facing=input.get(),shape=input.get(),power=input.get(),flags=input.get();
                if(facing<0||shape<0||power<0||flags<0||chunk.get(x,y,z)==BlockId::Air) return false;
                if(chunk.get_state(x,y,z)!=nullptr) return false;
                const BlockState state{static_cast<std::uint8_t>(facing),static_cast<std::uint8_t>(shape),
                                       static_cast<std::uint8_t>(power),static_cast<std::uint8_t>(flags)};
                if(!chunk.set_state(x,y,z,state)) return false;
            }
        }
    }
    if(input.peek()!=std::char_traits<char>::eof()) return false;
    chunks_.swap(loaded);
    return true;
}
}
