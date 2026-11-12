#include "ChunkMesh.hpp"
#include <array>

namespace mc::render {
namespace {
struct Face {
    int dx,dy,dz;
    float nx,ny,nz;
    std::array<std::array<float,3>,4> corners;
};
constexpr Face faces[6]={
    { 1,0,0, 1,0,0, {{{1,0,0},{1,1,0},{1,1,1},{1,0,1}}} },
    {-1,0,0,-1,0,0, {{{0,0,1},{0,1,1},{0,1,0},{0,0,0}}} },
    {0, 1,0, 0,1,0, {{{0,1,1},{1,1,1},{1,1,0},{0,1,0}}} },
    {0,-1,0, 0,-1,0,{{{0,0,0},{1,0,0},{1,0,1},{0,0,1}}} },
    {0,0, 1, 0,0,1, {{{1,0,1},{1,1,1},{0,1,1},{0,0,1}}} },
    {0,0,-1,0,0,-1,{{{0,0,0},{0,1,0},{1,1,0},{1,0,0}}} }
};
bool occludes(mc::world::BlockId id) {
    return id!=mc::world::BlockId::Air && id!=mc::world::BlockId::Water;
}
}
ChunkMeshData ChunkMeshBuilder::build(const mc::world::World& world,int cx,int cz,int sectionY) {
    ChunkMeshData out;
    const auto* chunk=world.find_chunk(cx,cz);
    if(!chunk || sectionY<0 || sectionY>=mc::world::Chunk::Height/SectionHeight) return out;
    out.source_revision=chunk->revision();
    out.vertices.reserve(4096);
    out.indices.reserve(6144);
    const int baseX=cx*mc::world::Chunk::Width;
    const int baseY=sectionY*SectionHeight;
    const int baseZ=cz*mc::world::Chunk::Depth;
    for(int y=0;y<SectionHeight;++y) for(int z=0;z<mc::world::Chunk::Depth;++z)
    for(int x=0;x<mc::world::Chunk::Width;++x) {
        const auto id=world.get_block(baseX+x,baseY+y,baseZ+z);
        if(!occludes(id)) continue;
        for(const Face& face:faces) {
            if(occludes(world.get_block(baseX+x+face.dx,baseY+y+face.dy,baseZ+z+face.dz))) continue;
            if(out.vertices.size()>65531u) return ChunkMeshData{};
            const auto first=static_cast<std::uint16_t>(out.vertices.size());
            for(const auto& corner:face.corners) {
                out.vertices.push_back(ChunkMeshVertex{
                    float(baseX+x)+corner[0],float(baseY+y)+corner[1],float(baseZ+z)+corner[2],
                    face.nx,face.ny,face.nz,static_cast<std::uint16_t>(id)
                });
            }
            out.indices.push_back(first);
            out.indices.push_back(static_cast<std::uint16_t>(first+1));
            out.indices.push_back(static_cast<std::uint16_t>(first+2));
            out.indices.push_back(first);
            out.indices.push_back(static_cast<std::uint16_t>(first+2));
            out.indices.push_back(static_cast<std::uint16_t>(first+3));
        }
    }
    return out;
}
}
