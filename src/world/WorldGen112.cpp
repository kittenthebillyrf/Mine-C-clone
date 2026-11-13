#include "WorldGen112.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mc::world {
namespace {
constexpr std::uint64_t MULT=0x5DEECE66DULL, ADD=0xBULL, MASK=(1ULL<<48)-1ULL;
double fade(double t) { return t*t*t*(t*(t*6.0-15.0)+10.0); }
double lerp(double t,double a,double b) { return a+t*(b-a); }
double grad(int hash,double x,double y,double z) {
    static constexpr double gx[16]={1,-1,1,-1,1,-1,1,-1,0,0,0,0,1,0,-1,0};
    static constexpr double gy[16]={1,1,-1,-1,0,0,0,0,1,-1,1,-1,1,-1,1,-1};
    static constexpr double gz[16]={0,0,0,0,1,1,-1,-1,1,1,-1,-1,0,1,0,-1};
    const int i=hash&15;
    return gx[i]*x+gy[i]*y+gz[i]*z;
}
int floor_int(double v) { const int i=static_cast<int>(v); return v<double(i)?i-1:i; }
}
void JavaRandom::set_seed(std::int64_t seed) { seed_=(static_cast<std::uint64_t>(seed)^MULT)&MASK; }
std::uint32_t JavaRandom::next_bits(int bits) { seed_=(seed_*MULT+ADD)&MASK; return static_cast<std::uint32_t>(seed_>>(48-bits)); }
int JavaRandom::next_int(int bound) {
    if(bound<=0) return 0;
    if((bound&-bound)==bound) return int((std::uint64_t(bound)*next_bits(31))>>31);
    std::uint32_t bits=0; int value=0;
    do {
        bits=next_bits(31);
        value=int(bits%std::uint32_t(bound));
        // java.util.Random performs this rejection test with signed 32-bit
        // overflow; preserve that behavior for bit-for-bit RNG compatibility.
    } while(static_cast<std::int32_t>(bits-std::uint32_t(value)+std::uint32_t(bound-1))<0);
    return value;
}
double JavaRandom::next_double() {
    const std::uint64_t a=std::uint64_t(next_bits(26))<<27;
    const std::uint64_t b=next_bits(27);
    return double(a+b)/double(1ULL<<53);
}
ImprovedNoise112::ImprovedNoise112(JavaRandom& random) {
    ox_=random.next_double()*256.0; oy_=random.next_double()*256.0; oz_=random.next_double()*256.0;
    for(int i=0;i<256;++i) permutation_[i]=i;
    for(int i=0;i<256;++i) {
        const int j=i+random.next_int(256-i);
        std::swap(permutation_[i],permutation_[j]);
        permutation_[i+256]=permutation_[i];
    }
}
double ImprovedNoise112::sample(double x,double y,double z) const {
    x+=ox_; y+=oy_; z+=oz_;
    const int ix=floor_int(x),iy=floor_int(y),iz=floor_int(z);
    const int X=ix&255,Y=iy&255,Z=iz&255;
    x-=ix; y-=iy; z-=iz;
    const double u=fade(x),v=fade(y),w=fade(z);
    const int A=permutation_[X]+Y, AA=permutation_[A]+Z, AB=permutation_[A+1]+Z;
    const int B=permutation_[X+1]+Y, BA=permutation_[B]+Z, BB=permutation_[B+1]+Z;
    const double x00=lerp(u,grad(permutation_[AA],x,y,z),grad(permutation_[BA],x-1,y,z));
    const double x10=lerp(u,grad(permutation_[AB],x,y-1,z),grad(permutation_[BB],x-1,y-1,z));
    const double x01=lerp(u,grad(permutation_[AA+1],x,y,z-1),grad(permutation_[BA+1],x-1,y,z-1));
    const double x11=lerp(u,grad(permutation_[AB+1],x,y-1,z-1),grad(permutation_[BB+1],x-1,y-1,z-1));
    return lerp(w,lerp(v,x00,x10),lerp(v,x01,x11));
}
OctaveNoise112::OctaveNoise112(JavaRandom& random,int count) {
    octaves_.reserve(std::max(0,count));
    for(int i=0;i<count;++i) octaves_.emplace_back(random);
}
double OctaveNoise112::sample(double x,double y,double z) const {
    // Match NoiseGeneratorOctaves.generateNoiseOctaves: each octave floors
    // the scaled X/Z offsets, wraps their integer parts at 2^24, and keeps
    // the fractional remainder before sampling the improved-noise field.
    double result=0.0;
    double octaveScale=1.0;
    for(const auto& octave:octaves_) {
        const double scaledX=x*octaveScale;
        const double scaledY=y*octaveScale;
        const double scaledZ=z*octaveScale;
        const auto floorX=static_cast<std::int64_t>(std::floor(scaledX));
        const auto floorZ=static_cast<std::int64_t>(std::floor(scaledZ));
        const double wrappedX=(scaledX-static_cast<double>(floorX))
            +static_cast<double>(floorX%16777216LL);
        const double wrappedZ=(scaledZ-static_cast<double>(floorZ))
            +static_cast<double>(floorZ%16777216LL);
        result+=octave.sample(wrappedX,scaledY,wrappedZ)/octaveScale;
        octaveScale*=0.5;
    }
    return result;
}
std::vector<double> OctaveNoise112::generate_region(int x_offset,int y_offset,int z_offset,
    int x_size,int y_size,int z_size,double x_scale,double y_scale,double z_scale) const {
    if(x_size<=0||y_size<=0||z_size<=0) return {};
    const std::size_t total=std::size_t(x_size)*std::size_t(y_size)*std::size_t(z_size);
    std::vector<double> values(total,0.0);
    double octaveScale=1.0;
    for(const auto& octave:octaves_) {
        const double startX=double(x_offset)*octaveScale*x_scale;
        const double startY=double(y_offset)*octaveScale*y_scale;
        const double startZ=double(z_offset)*octaveScale*z_scale;
        const auto floorX=static_cast<std::int64_t>(std::floor(startX));
        const auto floorZ=static_cast<std::int64_t>(std::floor(startZ));
        const double baseX=(startX-double(floorX))+double(floorX%16777216LL);
        const double baseZ=(startZ-double(floorZ))+double(floorZ%16777216LL);
        const double stepX=x_scale*octaveScale;
        const double stepY=y_scale*octaveScale;
        const double stepZ=z_scale*octaveScale;
        const double amplitude=1.0/octaveScale;
        std::size_t index=0;
        for(int x=0;x<x_size;++x) {
            const double nx=baseX+double(x)*stepX;
            for(int z=0;z<z_size;++z) {
                const double nz=baseZ+double(z)*stepZ;
                for(int y=0;y<y_size;++y) {
                    const double ny=startY+double(y)*stepY;
                    values[index++]+=octave.sample(nx,ny,nz)*amplitude;
                }
            }
        }
        octaveScale*=0.5;
    }
    return values;
}
OverworldGenerator112::OverworldGenerator112(std::int64_t seed) : seed_(seed) {
    JavaRandom random(seed);
    low_=std::make_unique<OctaveNoise112>(random,16);
    high_=std::make_unique<OctaveNoise112>(random,16);
    selector_=std::make_unique<OctaveNoise112>(random,8);
    surface_=std::make_unique<OctaveNoise112>(random,4);
    scale_=std::make_unique<OctaveNoise112>(random,10);
    depth_=std::make_unique<OctaveNoise112>(random,16);
    forest_=std::make_unique<OctaveNoise112>(random,8);
}
DensityMap112 OverworldGenerator112::generate_density_map(int chunk_x,int chunk_z,
    const std::array<BiomeTerrain112,100>& biome_grid) const {
    constexpr double coordinate_scale=684.412;
    constexpr double height_scale=684.412;
    constexpr double lower_limit_scale=512.0;
    constexpr double upper_limit_scale=512.0;
    constexpr double main_scale_x=80.0,main_scale_y=160.0,main_scale_z=80.0;
    constexpr double depth_scale_x=200.0,depth_scale_z=200.0;
    constexpr double base_size=8.5,stretch_y=12.0;
    std::array<double,25> biome_weights{};
    for(int dz=-2;dz<=2;++dz) for(int dx=-2;dx<=2;++dx)
        biome_weights[(dx+2)+(dz+2)*5]=10.0/std::sqrt(double(dx*dx+dz*dz)+0.2);

    const int ox=chunk_x*4,oz=chunk_z*4;
    const auto depth_region=depth_->generate_region(ox,10,oz,5,1,5,
        depth_scale_x,1.0,depth_scale_z);
    const auto main_region=selector_->generate_region(ox,0,oz,5,33,5,
        coordinate_scale/main_scale_x,height_scale/main_scale_y,coordinate_scale/main_scale_z);
    const auto min_region=low_->generate_region(ox,0,oz,5,33,5,
        coordinate_scale,height_scale,coordinate_scale);
    const auto max_region=high_->generate_region(ox,0,oz,5,33,5,
        coordinate_scale,height_scale,coordinate_scale);
    DensityMap112 result;
    std::size_t density_index=0,depth_index=0;
    for(int x=0;x<5;++x) for(int z=0;z<5;++z) {
        const BiomeTerrain112& center=biome_grid[(x+2)+(z+2)*10];
        double variation_sum=0.0,base_sum=0.0,weight_sum=0.0;
        for(int dz=-2;dz<=2;++dz) for(int dx=-2;dx<=2;++dx) {
            const BiomeTerrain112& biome=biome_grid[(x+dx+2)+(z+dz+2)*10];
            const double base=biome.base_height;
            const double variation=biome.height_variation;
            double weight=biome_weights[(dx+2)+(dz+2)*5]/(base+2.0);
            if(biome.base_height>center.base_height) weight*=0.5;
            variation_sum+=variation*weight;
            base_sum+=base*weight;
            weight_sum+=weight;
        }
        const double variation=((variation_sum/weight_sum)*0.9+0.1);
        const double base=((base_sum/weight_sum)*4.0-1.0)/8.0;
        double depth=depth_region[depth_index++]/8000.0;
        if(depth<0.0) depth=-depth*0.3;
        depth=depth*3.0-2.0;
        if(depth<0.0) {
            depth=std::max(-1.0,depth/2.0);
            depth=depth/1.4/2.0;
        } else {
            depth=std::min(1.0,depth)/8.0;
        }
        double adjusted_base=base+depth*0.2;
        adjusted_base=adjusted_base*base_size/8.0;
        const double vertical_center=base_size+adjusted_base*4.0;
        for(int y=0;y<33;++y) {
            double vertical=(double(y)-vertical_center)*stretch_y*128.0/256.0/variation;
            if(vertical<0.0) vertical*=4.0;
            const double low=min_region[density_index]/lower_limit_scale;
            const double high=max_region[density_index]/upper_limit_scale;
            const double selector=(main_region[density_index]/10.0+1.0)/2.0;
            const double blend=std::clamp(selector,0.0,1.0);
            double density=low+(high-low)*blend-vertical;
            if(y>29) {
                const double fade_top=double(y-29)/3.0;
                density=density*(1.0-fade_top)-10.0*fade_top;
            }
            result.values[density_index]=density;
            ++density_index;
        }
    }
    return result;
}

int OverworldGenerator112::terrain_height(int x,int z) const {
    const double broad=low_->sample(x/180.0,0.0,z/180.0);
    const double high=high_->sample(x/72.0,0.0,z/72.0);
    const double selector=selector_->sample(x/110.0,0.0,z/110.0);
    const double detail=scale_->sample(x/24.0,0.0,z/24.0);
    const double blend=std::clamp((selector+1.0)*0.5,0.0,1.0);
    const double continental=lerp(blend,broad*0.55,high*0.8);
    return std::clamp(int(63.0+continental*23.0+detail*2.0),4,120);
}
void OverworldGenerator112::generate_chunk(World& world,int cx,int cz) const {
    constexpr int sea=63;
    // Until the biome provider lands, use the exact vanilla Plains terrain
    // parameters for all 10x10 generation samples.
    std::array<BiomeTerrain112,100> biomes{};
    const DensityMap112 density=generate_density_map(cx,cz,biomes);
    const int ox=cx*16,oz=cz*16;

    // Java 1.12.2 expands each 5x33x5 density lattice cell into
    // 4x8x4 blocks using trilinear interpolation.
    for(int cell_x=0;cell_x<4;++cell_x) {
        for(int cell_z=0;cell_z<4;++cell_z) {
            for(int node_y=0;node_y<32;++node_y) {
                for(int sub_y=0;sub_y<8;++sub_y) {
                    const double fy=double(sub_y)/8.0;
                    for(int sub_x=0;sub_x<4;++sub_x) {
                        const double fx=double(sub_x)/4.0;
                        for(int sub_z=0;sub_z<4;++sub_z) {
                            const double fz=double(sub_z)/4.0;
                            const double a=lerp(lerp(density.at(cell_x,cell_z,node_y),
                                density.at(cell_x+1,cell_z,node_y),fx),
                                lerp(density.at(cell_x,cell_z+1,node_y),
                                density.at(cell_x+1,cell_z+1,node_y),fx),fz);
                            const double b=lerp(lerp(density.at(cell_x,cell_z,node_y+1),
                                density.at(cell_x+1,cell_z,node_y+1),fx),
                                lerp(density.at(cell_x,cell_z+1,node_y+1),
                                density.at(cell_x+1,cell_z+1,node_y+1),fx),fz);
                            const double value=lerp(a,b,fy);
                            const int x=cell_x*4+sub_x;
                            const int z=cell_z*4+sub_z;
                            const int y=node_y*8+sub_y;
                            const int world_x=ox+x,world_z=oz+z;
                            if(value>0.0) {
                                world.set_block(world_x,y,world_z,y==0?BlockId::Bedrock:BlockId::Stone);
                            } else if(y<=sea) {
                                world.set_block(world_x,y,world_z,BlockId::Water);
                            }
                        }
                    }
                }
            }
        }
    }

    // Temporary plains surface pass: keep the density terrain playable while
    // the full biome-specific surface replacement and cave stages are added.
    for(int z=0;z<16;++z) for(int x=0;x<16;++x) {
        const int wx=ox+x,wz=oz+z;
        int top=-1;
        for(int y=255;y>=0;--y) {
            const BlockId id=world.get_block(wx,y,wz);
            if(id==BlockId::Stone||id==BlockId::Bedrock) { top=y; break; }
        }
        if(top<0) continue;
        for(int depth=0;depth<4&&top-depth>0;++depth) {
            const int y=top-depth;
            world.set_block(wx,y,wz,depth==0?BlockId::Grass:BlockId::Dirt);
        }
    }
}
}
