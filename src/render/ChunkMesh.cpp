#include "ChunkMesh.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

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
// This mesher emits only axis-aligned full cubes. Non-cube blocks stay on
// the existing model renderer until their dedicated geometry is supported.
bool is_full_cube(mc::world::BlockId id) {
    using mc::world::BlockId;
    switch(id) {
        case BlockId::Air:
        case BlockId::Water:
        case BlockId::Lava:
        case BlockId::Torch:
        case BlockId::Snow:
        case BlockId::Cactus:
        case BlockId::OakSlab:
        case BlockId::SpruceSlab:
        case BlockId::BirchSlab:
        case BlockId::JungleSlab:
        case BlockId::AcaciaSlab:
        case BlockId::DarkOakSlab:
        case BlockId::OakFence:
        case BlockId::SpruceFence:
        case BlockId::BirchFence:
        case BlockId::JungleFence:
        case BlockId::AcaciaFence:
        case BlockId::DarkOakFence:
        case BlockId::OakStairs:
        case BlockId::SpruceStairs:
        case BlockId::BirchStairs:
        case BlockId::JungleStairs:
        case BlockId::AcaciaStairs:
        case BlockId::DarkOakStairs:
            return false;
        default:
            return mc::world::block_info(id).solid;
    }
}
bool occludes(mc::world::BlockId id) {
    return is_full_cube(id);
}
int axis_of(const std::array<float,3>& edge) {
    for(int axis=0;axis<3;++axis) if(edge[std::size_t(axis)]!=0.0f) return axis;
    return 0;
}
int sign_of(const std::array<float,3>& edge,int axis) {
    return edge[std::size_t(axis)]<0.0f?-1:1;
}
using Mask=std::array<std::uint16_t,256>;
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

    // Greedy meshing: for each face direction and slice, merge adjacent
    // coplanar faces only when their block IDs match.
    for(const Face& face:faces) {
        const std::array<float,3> edgeU{
            face.corners[3][0]-face.corners[0][0],
            face.corners[3][1]-face.corners[0][1],
            face.corners[3][2]-face.corners[0][2]};
        const std::array<float,3> edgeV{
            face.corners[1][0]-face.corners[0][0],
            face.corners[1][1]-face.corners[0][1],
            face.corners[1][2]-face.corners[0][2]};
        const int normalAxis=face.dx!=0?0:(face.dy!=0?1:2);
        const int uAxis=axis_of(edgeU),vAxis=axis_of(edgeV);
        const int uSign=sign_of(edgeU,uAxis),vSign=sign_of(edgeV,vAxis);
        for(int slice=0;slice<SectionHeight;++slice) {
            Mask mask{};
            for(int v=0;v<16;++v) for(int u=0;u<16;++u) {
                int p[3]{};
                p[normalAxis]=slice;
                p[uAxis]=uSign>0?u:15-u;
                p[vAxis]=vSign>0?v:15-v;
                const int wx=baseX+p[0],wy=baseY+p[1],wz=baseZ+p[2];
                const auto id=world.get_block(wx,wy,wz);
                if(!is_full_cube(id)) continue;
                if(occludes(world.get_block(wx+face.dx,wy+face.dy,wz+face.dz))) continue;
                mask[std::size_t(v*16+u)]=static_cast<std::uint16_t>(id);
            }
            for(int v=0;v<16;++v) for(int u=0;u<16;) {
                const std::size_t index=std::size_t(v*16+u);
                const std::uint16_t id=mask[index];
                if(id==0) { ++u; continue; }
                int width=1;
                while(u+width<16 && mask[std::size_t(v*16+u+width)]==id) ++width;
                int height=1;
                bool extend=true;
                while(v+height<16 && extend) {
                    for(int k=0;k<width;++k) {
                        if(mask[std::size_t((v+height)*16+u+k)]!=id) { extend=false; break; }
                    }
                    if(extend) ++height;
                }
                for(int row=0;row<height;++row)
                    for(int col=0;col<width;++col)
                        mask[std::size_t((v+row)*16+u+col)]=0;

                int p[3]{};
                p[normalAxis]=slice;
                p[uAxis]=uSign>0?u:15-u;
                p[vAxis]=vSign>0?v:15-v;
                const float origin[3]={
                    float(baseX+p[0])+face.corners[0][0],
                    float(baseY+p[1])+face.corners[0][1],
                    float(baseZ+p[2])+face.corners[0][2]};
                const float du[3]={edgeU[0]*float(width),edgeU[1]*float(width),edgeU[2]*float(width)};
                const float dv[3]={edgeV[0]*float(height),edgeV[1]*float(height),edgeV[2]*float(height)};
                if(out.vertices.size()>65531u) return ChunkMeshData{};
                const auto first=static_cast<std::uint16_t>(out.vertices.size());
                const float positions[4][3]={
                    {origin[0],origin[1],origin[2]},
                    {origin[0]+dv[0],origin[1]+dv[1],origin[2]+dv[2]},
                    {origin[0]+dv[0]+du[0],origin[1]+dv[1]+du[1],origin[2]+dv[2]+du[2]},
                    {origin[0]+du[0],origin[1]+du[1],origin[2]+du[2]}};
                for(const auto& pos:positions) {
                    out.vertices.push_back(ChunkMeshVertex{pos[0],pos[1],pos[2],
                        face.nx,face.ny,face.nz,id});
                }
                out.indices.push_back(first);
                out.indices.push_back(static_cast<std::uint16_t>(first+1));
                out.indices.push_back(static_cast<std::uint16_t>(first+2));
                out.indices.push_back(first);
                out.indices.push_back(static_cast<std::uint16_t>(first+2));
                out.indices.push_back(static_cast<std::uint16_t>(first+3));
                u+=width;
            }
        }
    }
    return out;
}
}
