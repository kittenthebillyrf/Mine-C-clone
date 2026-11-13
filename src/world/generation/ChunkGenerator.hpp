#pragma once
#include "World.hpp"
#include <cstdint>

namespace mc::world::generation {
enum class Biome : std::uint8_t { Plains, Forest, Desert, Taiga, Tundra, Swamp, Savanna };
struct ColumnSample { int height{}; Biome biome{Biome::Plains}; float temperature{}; float rainfall{}; };
class ChunkGenerator {
public:
    explicit ChunkGenerator(std::uint64_t seed) : seed_(seed) {}
    ColumnSample sample(int world_x, int world_z) const noexcept;
    int height_at(int world_x, int world_z) const noexcept { return sample(world_x, world_z).height; }
    void generate_chunk(World& world, int chunk_x, int chunk_z) const;
    std::uint64_t seed() const noexcept { return seed_; }
private:
    std::uint64_t seed_;
};
}
