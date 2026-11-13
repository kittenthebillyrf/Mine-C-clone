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
std::uint64_t hash3(int x,int y,int z,std::uint64_t seed) noexcept {
    return mix(seed ^ (std::uint64_t(std::uint32_t(x))*0x632be59bd9b4e019ULL)
                    ^ (std::uint64_t(std::uint32_t(y))*0x8cb92baa3f3d8dd7ULL)
                    ^ (std::uint64_t(std::uint32_t(z))*0x9e3779b185ebca87ULL));
}
double unit(std::uint64_t h) noexcept { return double(h>>11)*(1.0/9007199254740992.0); }
double lattice(int x,int z,std::uint64_t seed) noexcept { return unit(hash3(x,0,z,seed))*2.0-1.0; }
double fade(double t) noexcept { return t*t*t*(t*(t*6.0-15.0)+10.0); }
double lerp(double a,double b,double t) noexcept { return a+(b-a)*t; }
double value_noise(double x,double z,std::uint64_t seed) noexcept {
    const int ix=int(std::floor(x)), iz=int(std::floor(z));
    const double fx=fade(x-ix), fz=fade(z-iz);
    return lerp(lerp(lattice(ix,iz,seed),lattice(ix+1,iz,seed),fx),
                lerp(lattice(ix,iz+1,seed),lattice(ix+1,iz+1,seed),fx),fz);
}
double noise3(double x,double y,double z,std::uint64_t seed) noexcept {
    const int ix=int(std::floor(x)),iy=int(std::floor(y)),iz=int(std::floor(z));
    const double fx=fade(x-ix),fy=fade(y-iy),fz=fade(z-iz);
    double layer[2]{};
    for(int dy=0;dy<2;++dy) {
        const double a=lerp(unit(hash3(ix,iy+dy,iz,seed)),unit(hash3(ix+1,iy+dy,iz,seed)),fx);
        const double b=lerp(unit(hash3(ix,iy+dy,iz+1,seed)),unit(hash3(ix+1,iy+dy,iz+1,seed)),fx);
        layer[dy]=lerp(a,b,fz);
    }
    return lerp(layer[0],layer[1],fy)*2.0-1.0;
}
double fbm(double x,double z,std::uint64_t seed,int octaves,double frequency,double persistence=0.5) noexcept {
    double sum=0,amp=1,norm=0;
    for(int i=0;i<octaves;++i) {
        sum+=value_noise(x*frequency,z*frequency,mix(seed+std::uint64_t(i)*0x9e3779b9ULL))*amp;
        norm+=amp; amp*=persistence; frequency*=2.0;
    }
    return norm>0?sum/norm:0;
}
bool is_tree_biome(Biome b) noexcept { return b==Biome::Forest||b==Biome::Taiga||b==Biome::Savanna; }
}
ColumnSample ChunkGenerator::sample(int x,int z) const noexcept {
    const double continental=fbm(x,z,seed_^0x434f4e54494e454eULL,5,0.0018,0.55);
    const double erosion=fbm(x,z,seed_^0x45524f53494f4e21ULL,4,0.0065,0.52);
    const double ridge=1.0-std::abs(fbm(x,z,seed_^0x5249444745532121ULL,4,0.004,0.55));
    const double detail=fbm(x,z,seed_^0x44455441494c2121ULL,6,0.025,0.5);
    const double temperature=std::clamp(0.58-fbm(x,z,seed_^0x54454d5045524154ULL,4,0.0012)*0.38,0.0,1.0);
    const double rainfall=std::clamp(0.5+fbm(x,z,seed_^0x5241494e46414c4cULL,4,0.0015)*0.48,0.0,1.0);
    const double riverField=1.0-std::abs(fbm(x,z,seed_^0x5249564552532121ULL,3,0.0032,0.55));
    double elevation=63.0+continental*34.0-erosion*9.0+ridge*std::max(0.0,continental+0.15)*36.0+detail*5.0;
    if(riverField>0.91) elevation-=std::min(18.0,(riverField-0.91)*220.0);
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
        const bool gravelPatch=unit(hash3(ox+x,0,oz+z,seed_^0x47524156454c2121ULL))>0.965;
        for(int y=0;y<=c.height;++y) {
            BlockId id=BlockId::Stone;
            if(y<4) id=BlockId::Bedrock;
            else if(y>=c.height-1) id=gravelPatch?BlockId::Gravel:(sandy?BlockId::Sand:BlockId::Grass);
            else if(y>=c.height-4) id=sandy?BlockId::Sand:BlockId::Dirt;

            if(y>5&&y<c.height-5&&y<120) {
                const double broad=noise3((ox+x)/25.0,y/13.0,(oz+z)/25.0,seed_^0x4341564542524f41ULL);
                const double detail=noise3((ox+x)/11.0,y/7.0,(oz+z)/11.0,seed_^0x4341564544455441ULL);
                if(broad>0.42&&detail>-0.12) id=BlockId::Air;
            }

            if(id==BlockId::Stone&&y>5&&y<110) {
                const double ore=unit(hash3((ox+x)/2,y/2,(oz+z)/2,seed_^0x4f5245535041574eULL));
                if(y<52&&ore>0.982) id=BlockId::CoalOre;
                if(y<36&&ore>0.994) id=BlockId::IronOre;
            }
            world.set_block(ox+x,y,oz+z,id);
        }
        if(c.height<sea) {
            for(int y=c.height+1;y<=sea;++y) world.set_block(ox+x,y,oz+z,BlockId::Water);
        }
    }

    // Seeded, biome-aware tree placement. Each chunk owns its trunks and canopy.
    for(int x=2;x<Chunk::Width-2;++x) for(int z=2;z<Chunk::Depth-2;++z) {
        const int wx=ox+x,wz=oz+z;
        const auto c=columns[x][z];
        if(!is_tree_biome(c.biome)||c.height<=sea+1) continue;
        const double chance=unit(hash3(wx,0,wz,seed_^0x5452454553504157ULL));
        if(chance<0.965) continue;
        const int trunk=3+int(unit(hash3(wx,1,wz,seed_))*3.0);
        for(int y=1;y<=trunk;++y) world.set_block(wx,c.height-1+y,wz,BlockId::Wood);
        const int crownY=c.height-1+trunk;
        for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx) for(int dz=-2;dz<=2;++dz) {
            const int manhattan=std::abs(dx)+std::abs(dz)+std::max(0,std::abs(dy)-1);
            if(manhattan>4|| (dx==0&&dz==0&&dy<1)) continue;
            const int tx=wx+dx,tz=wz+dz,ty=crownY+dy;
            if(tx>=ox&&tx<ox+Chunk::Width&&tz>=oz&&tz<oz+Chunk::Depth&&
               world.get_block(tx,ty,tz)==BlockId::Air)
                world.set_block(tx,ty,tz,BlockId::Leaves);
        }
    }
}
}
