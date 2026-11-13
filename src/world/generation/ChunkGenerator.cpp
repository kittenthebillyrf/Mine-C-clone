#include "ChunkGenerator.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mc::world::generation {
namespace {
std::uint64_t mix(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
double lattice(int x, int z, std::uint64_t seed) noexcept {
    const auto h=mix(seed ^ (std::uint64_t(std::uint32_t(x))*0x632be59bd9b4e019ULL)
                          ^ (std::uint64_t(std::uint32_t(z))*0x9e3779b185ebca87ULL));
    return (double(h >> 11) * (1.0 / 9007199254740992.0))*2.0-1.0;
}
double fade(double t) noexcept { return t*t*t*(t*(t*6.0-15.0)+10.0); }
double lerp(double a,double b,double t) noexcept { return a+(b-a)*t; }
double value_noise(double x,double z,std::uint64_t seed) noexcept {
    const int ix=int(std::floor(x)), iz=int(std::floor(z));
    const double fx=fade(x-ix), fz=fade(z-iz);
    return lerp(lerp(lattice(ix,iz,seed),lattice(ix+1,iz,seed),fx),
                lerp(lattice(ix,iz+1,seed),lattice(ix+1,iz+1,seed),fx),fz);
}
double fbm(double x,double z,std::uint64_t seed,int octaves,double frequency,double persistence=0.5) noexcept {
    double sum=0,amp=1,norm=0;
    for(int i=0;i<octaves;++i) {
        sum+=value_noise(x*frequency,z*frequency,mix(seed+std::uint64_t(i)*0x9e3779b9ULL))*amp;
        norm+=amp; amp*=persistence; frequency*=2.0;
    }
    return norm>0?sum/norm:0;
}
double cave_noise(int x,int y,int z,std::uint64_t seed) noexcept {
    const auto h=mix(seed ^ (std::uint64_t(std::uint32_t(x))*0x632be59bd9b4e019ULL)
                          ^ (std::uint64_t(std::uint32_t(y))*0x8cb92baa3f3d8dd7ULL)
                          ^ (std::uint64_t(std::uint32_t(z))*0x9e3779b185ebca87ULL));
    return double(h >> 11)*(1.0/9007199254740992.0);
}
}
ColumnSample ChunkGenerator::sample(int x,int z) const noexcept {
    const double continental=fbm(x,z,seed_^0x434f4e54494e454eULL,5,0.0018,0.55);
    const double erosion=fbm(x,z,seed_^0x45524f53494f4e21ULL,4,0.0065,0.52);
    const double ridge=1.0-std::abs(fbm(x,z,seed_^0x5249444745532121ULL,4,0.004,0.55));
    const double detail=fbm(x,z,seed_^0x44455441494c2121ULL,6,0.025,0.5);
    const double temperature=std::clamp(0.58-fbm(x,z,seed_^0x54454d5045524154ULL,4,0.0012)*0.38,0.0,1.0);
    const double rainfall=std::clamp(0.5+fbm(x,z,seed_^0x5241494e46414c4cULL,4,0.0015)*0.48,0.0,1.0);
    double elevation=63.0+continental*34.0-erosion*9.0+ridge*std::max(0.0,continental+0.15)*36.0+detail*5.0;
    elevation=std::clamp(elevation,5.0,205.0);
    Biome biome=Biome::Plains;
    if(temperature<0.20) biome=Biome::Tundra;
    else if(temperature<0.34) biome=Biome::Taiga;
    else if(temperature>0.78&&rainfall<0.22) biome=Biome::Desert;
    else if(temperature>0.72&&rainfall<0.42) biome=Biome::Savanna;
    else if(rainfall>0.78&&elevation<76) biome=Biome::Swamp;
    else if(rainfall>0.52) biome=Biome::Forest;
    return {int(std::lround(elevation)),biome,float(temperature),float(rainfall)};
}
void ChunkGenerator::generate_chunk(World& world,int cx,int cz) const {
    constexpr int sea=62;
    const int ox=cx*Chunk::Width, oz=cz*Chunk::Depth;
    ColumnSample columns[Chunk::Width][Chunk::Depth];
    for(int x=0;x<Chunk::Width;++x) for(int z=0;z<Chunk::Depth;++z)
        columns[x][z]=sample(ox+x,oz+z);
    for(int x=0;x<Chunk::Width;++x) for(int z=0;z<Chunk::Depth;++z) {
        const auto c=columns[x][z];
        const bool sandy=c.biome==Biome::Desert||c.biome==Biome::Savanna||
                         (c.height<=sea+1&&c.temperature>0.48f);
        for(int y=0;y<=c.height;++y) {
            BlockId id=BlockId::Stone;
            if(y<4) id=BlockId::Bedrock;
            else if(y>=c.height-1) id=sandy?BlockId::Sand:BlockId::Grass;
            else if(y>=c.height-4) id=sandy?BlockId::Sand:BlockId::Dirt;
            if(y>4&&y<c.height-5&&y<120) {
                const double a=cave_noise((ox+x)/3,y/2,(oz+z)/3,seed_);
                const double b=cave_noise((ox+x)/7,y/4,(oz+z)/7,seed_^0x4341564553595354ULL);
                if(a>0.965&&b>0.54) id=BlockId::Air;
            }
            world.set_block(ox+x,y,oz+z,id);
        }
        if(c.height<sea) {
            for(int y=c.height+1;y<=sea;++y) {
                const BlockId fluid=(c.biome==Biome::Tundra&&y==sea)?BlockId::Water:BlockId::Water;
                world.set_block(ox+x,y,oz+z,fluid);
            }
        }
    }
}
}
