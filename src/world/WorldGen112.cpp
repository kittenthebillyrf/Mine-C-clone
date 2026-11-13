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
    const int ox=cx*16,oz=cz*16;

    // Climate selection follows the 1.12 warm/medium/cold biome families.
    // Terrain parameters are the registered biome base-height/variation values.
    auto biome_at=[&](int wx,int wz) {
        const double temperature=forest_->sample(wx/420.0,0.0,wz/420.0);
        const double rainfall=scale_->sample(wx/360.0,0.0,wz/360.0);
        BiomeTerrain112 b{};
        if(temperature>0.42) {
            b.id=BiomeId112::Desert; b.base_height=0.125f; b.height_variation=0.05f;
        } else if(temperature < -0.48) {
            b.id=BiomeId112::Snowy; b.base_height=0.125f; b.height_variation=0.05f;
        } else if(temperature < -0.12) {
            b.id=BiomeId112::Taiga; b.base_height=0.2f; b.height_variation=0.2f;
        } else if(rainfall>0.38) {
            b.id=BiomeId112::Forest; b.base_height=0.1f; b.height_variation=0.3f;
        } else if(rainfall < -0.45) {
            b.id=BiomeId112::Mountains; b.base_height=1.0f; b.height_variation=0.5f;
        } else if(rainfall < -0.15) {
            b.id=BiomeId112::Swamp; b.base_height=-0.2f; b.height_variation=0.1f;
        } else {
            b.id=BiomeId112::Plains; b.base_height=0.125f; b.height_variation=0.05f;
        }
        return b;
    };
    std::array<BiomeTerrain112,100> biome_grid{};
    for(int z=0;z<10;++z) for(int x=0;x<10;++x)
        biome_grid[x+z*10]=biome_at((cx*4+x-2)*4,(cz*4+z-2)*4);
    const DensityMap112 density=generate_density_map(cx,cz,biome_grid);

    // Expand the density lattice to 16x256x16 with the vanilla 4x8x4 subdivision.
    for(int cell_x=0;cell_x<4;++cell_x) for(int cell_z=0;cell_z<4;++cell_z)
        for(int node_y=0;node_y<32;++node_y) for(int sub_y=0;sub_y<8;++sub_y)
            for(int sub_x=0;sub_x<4;++sub_x) for(int sub_z=0;sub_z<4;++sub_z) {
                const double fy=double(sub_y)/8.0,fx=double(sub_x)/4.0,fz=double(sub_z)/4.0;
                const double a=lerp(lerp(density.at(cell_x,cell_z,node_y),density.at(cell_x+1,cell_z,node_y),fx),
                    lerp(density.at(cell_x,cell_z+1,node_y),density.at(cell_x+1,cell_z+1,node_y),fx),fz);
                const double b=lerp(lerp(density.at(cell_x,cell_z,node_y+1),density.at(cell_x+1,cell_z,node_y+1),fx),
                    lerp(density.at(cell_x,cell_z+1,node_y+1),density.at(cell_x+1,cell_z+1,node_y+1),fx),fz);
                const double value=lerp(a,b,fy);
                const int x=cell_x*4+sub_x,z=cell_z*4+sub_z,y=node_y*8+sub_y;
                if(value>0.0) world.set_block(ox+x,y,oz+z,y==0?BlockId::Bedrock:BlockId::Stone);
                else if(y<=sea) world.set_block(ox+x,y,oz+z,BlockId::Water);
            }

    // Ensure every terrain column has a continuous stone body down to the
    // bedrock floor, as in Java 1.12. The density interpolation above still
    // supplies the detailed terrain where it is positive; this column pass
    // fills gaps left by incomplete density/noise coverage without replacing
    // existing terrain. Caves are carved afterwards.
    for(int x=0;x<16;++x) for(int z=0;z<16;++z) {
        const int wx=ox+x,wz=oz+z;
        const int surface_y=std::clamp(terrain_height(wx,wz),5,120);
        for(int y=5;y<=surface_y;++y) {
            const BlockId current=world.get_block(wx,y,wz);
            if(current==BlockId::Air||current==BlockId::Water)
                world.set_block(wx,y,wz,BlockId::Stone);
        }
        for(int y=surface_y+1;y<=sea;++y) {
            if(world.get_block(wx,y,wz)==BlockId::Air)
                world.set_block(wx,y,wz,BlockId::Water);
        }
    }

    // Surface layers: desert sand, snowy snow, and grass/dirt elsewhere.
    const auto surface_noise=surface_->generate_region(ox,0,oz,16,1,16,0.0625,1.0,0.0625);
    const std::uint64_t chunk_seed=std::uint64_t(cx)*341873128712ULL+std::uint64_t(cz)*132897987541ULL;
    JavaRandom surface_random(static_cast<std::int64_t>(chunk_seed));
    for(int x=0;x<16;++x) for(int z=0;z<16;++z) {
        const int wx=ox+x,wz=oz+z;
        const BiomeTerrain112 biome=biome_at(wx*4,wz*4);
        const int depth=int(surface_noise[std::size_t(x)*16+z]/3.0+3.0+surface_random.next_double()*0.25);
        int remaining=-1;
        // Java 1.12 chooses one bedrock depth per X/Z column, not a new
        // random threshold at every Y. Sampling inside the Y loop creates
        // scattered floating bedrock layers throughout the entire world.
        const int bedrockDepth=surface_random.next_int(5);
        for(int y=255;y>=0;--y) {
            if(y<=bedrockDepth) { world.set_block(wx,y,wz,BlockId::Bedrock); continue; }
            const BlockId current=world.get_block(wx,y,wz);
            if(current==BlockId::Air) remaining=-1;
            else if(current==BlockId::Stone) {
                if(remaining==-1) {
                    const bool desert=biome.id==BiomeId112::Desert;
                    const bool snowy=biome.id==BiomeId112::Snowy;
                    remaining=depth;
                    const BlockId top=desert?BlockId::Sand:(snowy?BlockId::Snow:BlockId::Grass);
                    const BlockId filler=desert?BlockId::Sand:(snowy?BlockId::Dirt:BlockId::Dirt);
                    if(depth<=0) world.set_block(wx,y,wz,y<sea?BlockId::Water:BlockId::Stone);
                    else if(y>=sea-1) world.set_block(wx,y,wz,top);
                    else if(y<sea-7-depth) world.set_block(wx,y,wz,BlockId::Gravel);
                    else world.set_block(wx,y,wz,filler);
                } else if(remaining>0) {
                    --remaining;
                    world.set_block(wx,y,wz,biome.id==BiomeId112::Desert?BlockId::Sand:BlockId::Dirt);
                }
            }
        }
    }

    // Deterministic cave worms. Each chunk owns its random stream so generation
    // order does not change the result; water pockets are left untouched.
    JavaRandom cave_random(static_cast<std::int64_t>(chunk_seed ^ std::uint64_t(seed_) ^ 0xCA7E112ULL));
    const int cave_count=cave_random.next_int(4);
    for(int cave=0;cave<cave_count;++cave) {
        double x=double(ox+cave_random.next_int(16)),y=double(12+cave_random.next_int(48));
        double z=double(oz+cave_random.next_int(16));
        double angle=cave_random.next_double()*6.28318530718;
        double pitch=(cave_random.next_double()-0.5)*0.25;
        const int length=24+cave_random.next_int(40);
        for(int step=0;step<length;++step) {
            const double radius=1.2+std::sin(double(step)/length*3.14159265359)*2.0;
            x+=std::cos(angle); z+=std::sin(angle); y+=pitch;
            angle+=(cave_random.next_double()-0.5)*0.22;
            pitch=(pitch+(cave_random.next_double()-0.5)*0.08)*0.8;
            for(int bx=std::max(ox,int(x-radius));bx<=std::min(ox+15,int(x+radius));++bx)
                for(int bz=std::max(oz,int(z-radius));bz<=std::min(oz+15,int(z+radius));++bz)
                    for(int by=std::max(5,int(y-radius));by<=std::min(120,int(y+radius));++by) {
                        const double dx=(bx+0.5-x)/radius,dz=(bz+0.5-z)/radius,dy=(by+0.5-y)/radius;
                        if(dx*dx+dy*dy+dz*dz<1.0) {
                            const BlockId old=world.get_block(bx,by,bz);
                            if(old==BlockId::Stone||old==BlockId::Dirt||old==BlockId::Grass)
                                world.set_block(bx,by,bz,by<10?BlockId::Lava:BlockId::Air);
                        }
                    }
        }
    }

    // Ore veins: compact ellipsoidal clusters at depth bands inspired by
    // vanilla 1.12 distribution; independent chunk seed for reproducibility.
    JavaRandom ore_random(static_cast<std::int64_t>(chunk_seed ^ std::uint64_t(seed_) ^ 0x0AE112ULL));
    struct OreRule { BlockId id; int veins; int size; int min_y; int max_y; };
    constexpr OreRule ores[]={{BlockId::CoalOre,20,9,8,128},{BlockId::IronOre,20,8,8,64},
        {BlockId::GoldOre,2,8,4,32},{BlockId::RedstoneOre,8,7,4,16},
        {BlockId::LapisOre,1,7,4,32},{BlockId::DiamondOre,1,7,4,16},
        {BlockId::EmeraldOre,1,3,4,32}};
    for(const auto& ore:ores) for(int vein=0;vein<ore.veins;++vein) {
        const int cx0=ox+ore_random.next_int(16),cz0=oz+ore_random.next_int(16);
        const int cy=ore.min_y+ore_random.next_int(ore.max_y-ore.min_y+1);
        const int radius=std::max(1,ore.size/2);
        for(int dx=-radius;dx<=radius;++dx) for(int dy=-radius;dy<=radius;++dy)
            for(int dz=-radius;dz<=radius;++dz) {
                if(dx*dx+dy*dy+dz*dz>radius*radius) continue;
                const int wx=cx0+dx,wy=cy+dy,wz=cz0+dz;
                if(wx<ox||wx>=ox+16||wz<oz||wz>=oz+16) continue;
                if(world.get_block(wx,wy,wz)==BlockId::Stone)
                    world.set_block(wx,wy,wz,ore.id);
            }
    }

    // Deterministic biome vegetation pass. Trees are kept inside their
    // owning chunk so chunk generation order cannot leave half-built canopies.
    JavaRandom tree_random(static_cast<std::int64_t>(chunk_seed ^ std::uint64_t(seed_) ^ 0x7AEE112ULL));
    const BiomeTerrain112 vegetation_biome=biome_at(ox+8,oz+8);
    int tree_attempts=0;
    switch(vegetation_biome.id) {
        case BiomeId112::Forest: tree_attempts=7; break;
        case BiomeId112::Taiga: tree_attempts=5; break;
        case BiomeId112::Plains: tree_attempts=1; break;
        default: break;
    }
    for(int attempt=0;attempt<tree_attempts;++attempt) {
        const int tx=tree_random.next_int(12)+2,tz=tree_random.next_int(12)+2;
        int ground=-1;
        for(int y=254;y>sea-4;--y) {
            const BlockId id=world.get_block(ox+tx,y,oz+tz);
            if(id==BlockId::Grass||id==BlockId::Dirt) { ground=y; break; }
            if(id!=BlockId::Air&&id!=BlockId::Snow) break;
        }
        if(ground<sea-1) continue;
        const bool spruce=vegetation_biome.id==BiomeId112::Taiga;
        const int trunkHeight=(spruce?5:4)+tree_random.next_int(3);
        const BlockId log=spruce?BlockId::SpruceLog:BlockId::OakLog;
        const BlockId leaves=spruce?BlockId::SpruceLeaves:BlockId::OakLeaves;
        bool clear=true;
        for(int y=1;y<=trunkHeight+2;++y)
            if(world.get_block(ox+tx,ground+y,oz+tz)!=BlockId::Air) { clear=false; break; }
        if(!clear) continue;
        for(int y=1;y<=trunkHeight;++y) world.set_block(ox+tx,ground+y,oz+tz,log);
        for(int layer=trunkHeight-2;layer<=trunkHeight+1;++layer) {
            const int radius=layer==trunkHeight+1?1:2;
            for(int dx=-radius;dx<=radius;++dx) for(int dz=-radius;dz<=radius;++dz) {
                if(std::abs(dx)==radius&&std::abs(dz)==radius&&tree_random.next_int(3)==0) continue;
                const int lx=tx+dx,lz=tz+dz;
                if(lx<0||lx>=16||lz<0||lz>=16) continue;
                const int wy=ground+layer;
                if(world.get_block(ox+lx,wy,oz+lz)==BlockId::Air)
                    world.set_block(ox+lx,wy,oz+lz,leaves);
            }
        }
    }

    // Small deterministic village hut markers in suitable plains/desert chunks.
    // Full multi-piece village roads/structures remain a separate structure stage.
    const BiomeTerrain112 center_biome=biome_at(ox+8,oz+8);
    if((center_biome.id==BiomeId112::Plains||center_biome.id==BiomeId112::Desert)
        && ((std::uint64_t(cx)*31u+std::uint64_t(cz)*17u+std::uint64_t(seed_))%23u)==0u) {
        const int hx=ox+5,hz=oz+5;
        int hy=sea+1;
        for(int y=255;y>=1;--y) if(world.get_block(hx,y,hz)!=BlockId::Air&&world.get_block(hx,y,hz)!=BlockId::Water) { hy=y+1; break; }
        for(int x=0;x<5;++x) for(int z=0;z<5;++z) world.set_block(hx+x,hy-1,hz+z,BlockId::OakPlanks);
        for(int x=0;x<5;++x) for(int z=0;z<5;++z) for(int y=0;y<4;++y) {
            const bool wall=x==0||x==4||z==0||z==4;
            if(wall&&y<3) world.set_block(hx+x,hy+y,hz+z,BlockId::OakLog);
            else if(y==3) world.set_block(hx+x,hy+y,hz+z,BlockId::OakPlanks);
        }
        world.set_block(hx+2,hy,hz,BlockId::Air);
        world.set_block(hx+2,hy+1,hz,BlockId::Air);
    }
}
}
