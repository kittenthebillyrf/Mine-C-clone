#pragma once
#include "World.hpp"
#include <array>
#include <cstdint>
#include <vector>
#include <memory>

namespace mc::world {

// Java's 48-bit java.util.Random, retained so world seeds and permutation tables
// can match the 1.12.2 noise implementation.
class JavaRandom {
public:
    explicit JavaRandom(std::int64_t seed=0) { set_seed(seed); }
    void set_seed(std::int64_t seed);
    std::uint32_t next_bits(int bits);
    int next_int(int bound);
    double next_double();
private:
    std::uint64_t seed_=0;
};

class ImprovedNoise112 {
public:
    explicit ImprovedNoise112(JavaRandom& random);
    double sample(double x,double y,double z) const;
private:
    std::array<int,512> permutation_{};
    double ox_=0,oy_=0,oz_=0;
};

class OctaveNoise112 {
public:
    OctaveNoise112(JavaRandom& random,int octaves);
    double sample(double x,double y,double z) const;
    // Java 1.12.2 generateNoiseOctaves layout: X, Z, then Y.
    std::vector<double> generate_region(int x_offset,int y_offset,int z_offset,
        int x_size,int y_size,int z_size,double x_scale,double y_scale,double z_scale) const;
private:
    std::vector<ImprovedNoise112> octaves_;
};

// First portable Overworld terrain pass. Noise/RNG primitives mirror the
// 1.12.2 implementation; biome surface replacement, caves, structures and
// decoration are separate generation stages.
struct BiomeTerrain112 {
    float base_height=0.125f;
    float height_variation=0.05f;
};
struct DensityMap112 {
    std::array<double,5*5*33> values{};
    double at(int x,int z,int y) const { return values[(x*5+z)*33+y]; }
};

class OverworldGenerator112 {
public:
    explicit OverworldGenerator112(std::int64_t seed);
    void generate_chunk(World& world,int chunk_x,int chunk_z) const;
    int terrain_height(int world_x,int world_z) const;
    DensityMap112 generate_density_map(int chunk_x,int chunk_z,
        const std::array<BiomeTerrain112,100>& biome_grid) const;
private:
    std::int64_t seed_;
    std::unique_ptr<OctaveNoise112> low_;
    std::unique_ptr<OctaveNoise112> high_;
    std::unique_ptr<OctaveNoise112> selector_;
    // Vanilla 1.12.2 constructs a 4-octave surface Perlin generator here;
    // reserve the same RNG sequence before scale/depth/forest generators.
    std::unique_ptr<OctaveNoise112> surface_;
    std::unique_ptr<OctaveNoise112> scale_;
    std::unique_ptr<OctaveNoise112> depth_;
    std::unique_ptr<OctaveNoise112> forest_;
};
}
