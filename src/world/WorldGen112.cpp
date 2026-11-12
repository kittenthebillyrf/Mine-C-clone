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
    do { bits=next_bits(31); value=int(bits%std::uint32_t(bound)); } while(std::int64_t(bits)-value+(bound-1)<0);
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
    double result=0.0,scale=1.0;
    for(const auto& octave:octaves_) { result+=octave.sample(x*scale,y*scale,z*scale)/scale; scale*=0.5; }
    return result;
}
OverworldGenerator112::OverworldGenerator112(std::int64_t seed)
    : seed_(seed), low_(*([&]()->JavaRandom*{ static JavaRandom r; r.set_seed(seed); return &r; })(),16),
      high_(*([&]()->JavaRandom*{ static JavaRandom r; r.set_seed(seed ^ 0x9e3779b97f4a7c15LL); return &r; })(),16),
      selector_(*([&]()->JavaRandom*{ static JavaRandom r; r.set_seed(seed ^ 0x632be59bd9b4e019LL); return &r; })(),8),
      detail_(*([&]()->JavaRandom*{ static JavaRandom r; r.set_seed(seed ^ 0x85157af5ULL); return &r; })(),4) {}
int OverworldGenerator112::terrain_height(int x,int z) const {
    const double broad=low_.sample(x/180.0,0.0,z/180.0);
    const double high=high_.sample(x/72.0,0.0,z/72.0);
    const double selector=selector_.sample(x/110.0,0.0,z/110.0);
    const double detail=detail_.sample(x/24.0,0.0,z/24.0);
    const double blend=std::clamp((selector+1.0)*0.5,0.0,1.0);
    const double continental=lerp(blend,broad*0.55,high*0.8);
    return std::clamp(int(63.0+continental*23.0+detail*2.0),4,120);
}
void OverworldGenerator112::generate_chunk(World& world,int cx,int cz) const {
    constexpr int sea=63;
    const int ox=cx*16,oz=cz*16;
    for(int z=0;z<16;++z) for(int x=0;x<16;++x) {
        const int wx=ox+x,wz=oz+z;
        const int h=terrain_height(wx,wz);
        const double temperature=detail_.sample(wx/420.0,0,wz/420.0);
        const bool cold=temperature < -0.35;
        const bool dry=temperature > 0.45;
        for(int y=0;y<=h;++y) {
            BlockId id=BlockId::Stone;
            if(y==0) id=BlockId::Bedrock;
            else if(y>=h-3) {
                if(y==h) id=cold?BlockId::Sand:(dry?BlockId::Sand:BlockId::Grass);
                else id=(dry||h<=sea+1)?BlockId::Sand:BlockId::Dirt;
            }
            world.set_block(wx,y,wz,id);
        }
        for(int y=h+1;y<=sea;++y) world.set_block(wx,y,wz,BlockId::Water);
    }
}
}
