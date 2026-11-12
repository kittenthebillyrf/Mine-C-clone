#pragma once
#include "../world/World.hpp"
#include <cstdint>
#include <vector>

namespace mc::render {
struct ChunkMeshVertex {
    float x=0, y=0, z=0;
    float nx=0, ny=0, nz=0;
    std::uint16_t block_id=0;
};
struct ChunkMeshData {
    std::vector<ChunkMeshVertex> vertices;
    std::vector<std::uint16_t> indices;
    std::uint64_t source_revision=0;
    bool empty() const { return indices.empty(); }
};
class ChunkMeshBuilder {
public:
    static constexpr int SectionHeight=16;
    static ChunkMeshData build(const mc::world::World& world,int chunk_x,int chunk_z,int section_y);
};
}
