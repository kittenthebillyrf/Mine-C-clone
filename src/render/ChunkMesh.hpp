#pragma once
#include "../world/World.hpp"
#include <array>
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
    // Revisions of west, east, north and south chunks used for boundary culling.
    std::array<std::uint64_t,4> neighbor_revisions{};
    bool is_current(const mc::world::World& world,int chunk_x,int chunk_z) const;
    bool empty() const { return indices.empty(); }
};
class ChunkMeshBuilder {
public:
    static constexpr int SectionHeight=16;
    static bool is_full_cube_block(mc::world::BlockId id);
    static ChunkMeshData build(const mc::world::World& world,int chunk_x,int chunk_z,int section_y);
};
}
