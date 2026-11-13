#include "i18n/Localization.hpp"
#include "world/World.hpp"
#include "world/WorldGen112.hpp"
#include "render/TerrainViewer.hpp"
#include "render/ChunkMesh.hpp"
#include "assets/Json.hpp"
#include "assets/BlockModel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::uint32_t hash2(int x, int z, std::uint32_t seed) {
    std::uint32_t h = seed ^ (std::uint32_t(x) * 0x8da6b343u) ^ (std::uint32_t(z) * 0xd8163841u);
    h ^= h >> 13; h *= 0x85ebca6bu; h ^= h >> 16; h *= 0xc2b2ae35u; h ^= h >> 16;
    return h;
}
double fade(double t) { return t*t*t*(t*(t*6.0-15.0)+10.0); }
double lerp(double a,double b,double t) { return a+(b-a)*t; }
double lattice(int x,int z,std::uint32_t seed) {
    return (double(hash2(x,z,seed))/double(UINT32_MAX))*2.0-1.0;
}
double noise(double x,double z,std::uint32_t seed) {
    const int ix=int(std::floor(x)), iz=int(std::floor(z));
    const double fx=fade(x-ix), fz=fade(z-iz);
    return lerp(lerp(lattice(ix,iz,seed),lattice(ix+1,iz,seed),fx),
                lerp(lattice(ix,iz+1,seed),lattice(ix+1,iz+1,seed),fx),fz);
}
double terrain(int x,int z,std::uint32_t seed) {
    double n=0.0, amp=1.0, freq=0.012, total=0.0;
    for(int octave=0;octave<5;++octave) {
        n += noise(x*freq,z*freq,seed+std::uint32_t(octave)*1013u)*amp;
        total += amp; amp *= 0.5; freq *= 2.0;
    }
    const double continental=noise(x*0.003,z*0.003,seed^0x9e3779b9u);
    return std::clamp(62.0 + (n/total)*25.0 + continental*13.0, 4.0, 120.0);
}
std::string replace_all(std::string value, const std::string& token, const std::string& replacement) {
    std::size_t pos=0;
    while((pos=value.find(token,pos))!=std::string::npos) {
        value.replace(pos,token.size(),replacement);
        pos += replacement.size();
    }
    return value;
}
bool run_json_self_test() {
    const auto parsed=mc::assets::json::parse(R"({"parent":"block/cube","elements":[{"from":[0,0,0],"to":[16,16,16]}],"ambientocclusion":false,"scale":[1.0,0.5,1e0],"unicode":"\u041c\u0430\u0439\u043d"})");
    if(!parsed||!parsed.value.is_object())return false;
    const auto* parent=parsed.value.get("parent");const auto* elements=parsed.value.get("elements");
    const auto* ao=parsed.value.get("ambientocclusion");const auto* scale=parsed.value.get("scale");const auto* unicode=parsed.value.get("unicode");
    if(!parent||!parent->string()||*parent->string()!="block/cube")return false;
    if(!elements||!elements->array()||elements->array()->size()!=1)return false;
    const auto* from=elements->at(0)->get("from");
    if(!from||!from->array()||!from->at(2)->number()||*from->at(2)->number()!=0.0)return false;
    if(!ao||!ao->boolean()||*ao->boolean())return false;
    if(!scale||!scale->at(1)->number()||std::abs(*scale->at(1)->number()-0.5)>1e-12)return false;
    if(!unicode||!unicode->string()||*unicode->string()!="Майн")return false;
    return !mc::assets::json::parse(R"({"bad":01})") &&
           !mc::assets::json::parse(R"({"x":1,"x":2})") &&
           !mc::assets::json::parse("[1,]");
}
bool run_block_model_self_test() {
    const auto parsed=mc::assets::parse_block_model(R"({
      "parent":"block/cube_all",
      "ambientocclusion":false,
      "textures":{"all":"minecraft:blocks/stone","particle":"#all"},
      "elements":[{"from":[0,0,0],"to":[16,16,16],"shade":true,
        "faces":{"down":{"texture":"#all","uv":[0,0,16,16]},
                 "up":{"texture":"#all","rotation":90},
                 "north":{"texture":"#all","cullface":"north","tintindex":2}}}]
    })");
    if(!parsed||parsed.model.parent!="block/cube_all"||parsed.model.ambient_occlusion||
       parsed.model.textures.size()!=2||!parsed.model.has_elements||parsed.model.elements.size()!=1)return false;
    const auto& element=parsed.model.elements.front();
    if(element.faces.size()!=3||element.faces.at("up").rotation!=90||
       element.faces.at("north").tint_index!=2||element.faces.at("north").cull_face!="north")return false;
    const auto rotated=mc::assets::parse_block_model(R"({"elements":[{"from":[0,0,0],"to":[16,16,16],"rotation":{"origin":[8,8,8],"axis":"y","angle":22.5,"rescale":true},"faces":{"up":{"texture":"#top"}}}]})");
    if(!rotated||!rotated.model.elements.front().has_rotation||
       !rotated.model.elements.front().rotation.rescale)return false;
    return !mc::assets::parse_block_model(R"({"elements":[{"from":[0,0,0],"to":[16,16,16],"faces":{"side":{"texture":"#x"}}}]})");
}
bool run_world_self_test() {
    // java.util.Random reference values for seed 0.
    mc::world::JavaRandom javaRandom(0);
    if(std::abs(javaRandom.next_double()-0.730967787376657) > 1e-15) return false;
    mc::world::JavaRandom boundedRandom(0);
    if(boundedRandom.next_int(100)!=60) return false;
    mc::world::World world;
    if(world.get_block(0,10,0)!=mc::world::BlockId::Air) return false;
    if(!world.set_block(0,10,0,mc::world::BlockId::Stone)) return false;
    if(!world.set_block(-1,20,-17,mc::world::BlockId::Grass)) return false;
    if(world.get_block(0,10,0)!=mc::world::BlockId::Stone) return false;
    if(world.get_block(-1,20,-17)!=mc::world::BlockId::Grass) return false;
    if(world.chunk_count()!=2) return false;
    if(world.set_block(0,256,0,mc::world::BlockId::Dirt)) return false;
    // Every newly registered wood variant must have a stable, solid block definition.
    for(std::uint16_t raw=100;raw<115;++raw) {
        const auto id=static_cast<mc::world::BlockId>(raw);
        const auto& info=mc::world::block_info(id);
        if(info.id!=id || info.name.empty() || !info.solid) return false;
    }
    if(!world.set_block(4,12,4,mc::world::BlockId::DarkOakStairs)) return false;
    const mc::world::BlockState stairState{1,1,0,0};
    if(!world.set_block_state(4,12,4,stairState)) return false;
    if(!world.get_block_state(4,12,4) || world.get_block_state(4,12,4)->facing!=1 ||
       world.get_block_state(4,12,4)->shape!=1) return false;
    // Cover all four horizontal stair facings and both vertical halves.
    for(std::uint8_t facing=0;facing<4;++facing) {
        const int x=5+int(facing);
        if(!world.set_block(x,12,4,mc::world::BlockId::OakStairs) ||
           !world.set_block_state(x,12,4,mc::world::BlockState{facing,static_cast<std::uint8_t>(facing&1u),0,0}))
            return false;
    }
    // Falling-block simulation: sand and gravel descend one cell per tick,
    // replace liquids, and stop on a solid support.
    if(!world.set_block(2,0,2,mc::world::BlockId::Stone) ||
       !world.set_block(3,0,2,mc::world::BlockId::Stone) ||
       !world.set_block(2,3,2,mc::world::BlockId::Sand) ||
       !world.set_block(3,3,2,mc::world::BlockId::Gravel) ||
       !world.set_block(3,2,2,mc::world::BlockId::Water)) return false;
    if(world.tick_falling_blocks()!=2) return false;
    if(world.get_block(2,2,2)!=mc::world::BlockId::Sand ||
       world.get_block(3,2,2)!=mc::world::BlockId::Gravel ||
       world.get_block(3,3,2)!=mc::world::BlockId::Air) return false;
    if(world.tick_falling_blocks()!=2) return false;
    if(world.get_block(2,1,2)!=mc::world::BlockId::Sand ||
       world.get_block(3,1,2)!=mc::world::BlockId::Gravel) return false;
    if(world.tick_falling_blocks()!=0) return false;
    std::stringstream saved(std::ios::in|std::ios::out|std::ios::binary);
    if(!world.save(saved)) return false;
    mc::world::World restored;
    saved.seekg(0);
    if(!restored.load(saved)) return false;
    if(restored.chunk_count()!=2 ||
       restored.get_block(0,10,0)!=mc::world::BlockId::Stone ||
       restored.get_block(-1,20,-17)!=mc::world::BlockId::Grass ||
       restored.get_block(4,12,4)!=mc::world::BlockId::DarkOakStairs) return false;
    const auto* restoredStair=restored.get_block_state(4,12,4);
    if(!restoredStair || restoredStair->facing!=1 || restoredStair->shape!=1) return false;
    for(std::uint8_t facing=0;facing<4;++facing) {
        const auto* state=restored.get_block_state(5+int(facing),12,4);
        if(!state || state->facing!=facing || state->shape!=(facing&1u)) return false;
    }
    if(!restored.set_block(4,12,4,mc::world::BlockId::Air) ||
       restored.get_block_state(4,12,4)!=nullptr) return false;
    return mc::world::block_info(mc::world::BlockId::Stone).solid;
}

bool run_chunk_mesh_self_test() {
    mc::world::World world;
    for(int y=0;y<mc::render::ChunkMeshBuilder::SectionHeight;++y)
        for(int z=0;z<mc::world::Chunk::Depth;++z)
            for(int x=0;x<mc::world::Chunk::Width;++x)
                if(!world.set_block(x,y,z,mc::world::BlockId::Stone)) return false;
    const auto mesh=mc::render::ChunkMeshBuilder::build(world,0,0,0);
    if(mesh.vertices.size()!=24 || mesh.indices.size()!=36 ||
       mesh.source_revision!=world.chunk_revision(0,0)) return false;

    // Partial blocks and liquids must remain on the model renderer path;
    // they must never be approximated as full cubes by this mesher.
    mc::world::World partialWorld;
    if(!partialWorld.set_block(1,1,1,mc::world::BlockId::OakSlab) ||
       !partialWorld.set_block(2,1,1,mc::world::BlockId::OakFence) ||
       !partialWorld.set_block(3,1,1,mc::world::BlockId::OakStairs) ||
       !partialWorld.set_block(4,1,1,mc::world::BlockId::Cactus) ||
       !partialWorld.set_block(5,1,1,mc::world::BlockId::Water) ||
       !partialWorld.set_block(6,1,1,mc::world::BlockId::Lava)) return false;
    if(!mc::render::ChunkMeshBuilder::build(partialWorld,0,0,0).empty()) return false;

    // A non-cube neighbour cannot incorrectly hide a full-cube face.
    if(!partialWorld.set_block(0,1,1,mc::world::BlockId::Stone)) return false;
    const auto mixed=mc::render::ChunkMeshBuilder::build(partialWorld,0,0,0);
    return mixed.vertices.size()==24 && mixed.indices.size()==36;
}
}
int main(int argc,char** argv) {
    int size=128; std::uint32_t seed=1122;
    std::string output="terrain.ppm", language="en";
    bool self_test=false, open_3d=false, export_map=false;
    for(int i=1;i<argc;++i) {
        const std::string a=argv[i];
        if(a=="--size" && i+1<argc) size=std::clamp(std::atoi(argv[++i]),16,2048);
        else if(a=="--seed" && i+1<argc) seed=std::uint32_t(std::strtoul(argv[++i],nullptr,10));
        else if(a=="--output" && i+1<argc) output=argv[++i];
        else if(a=="--lang" && i+1<argc) language=argv[++i];
        else if(a=="--self-test") self_test=true;
        else if(a=="--3d") open_3d=true;
        else if(a=="--map") export_map=true;
        else if(a=="--help") {
            std::cout<<mc::i18n::text("app.title",language)<<"\n"
                     <<mc::i18n::text("help.usage",language)<<"\n"
                     <<mc::i18n::text("help.languages",language)<<"\n"
                     <<"3D preview: --3d\n";
            return 0;
        }
    }
    if(!mc::i18n::is_supported(language)) {
        std::cerr<<replace_all(mc::i18n::text("language.unknown","en"),"{code}",language)<<"\n";
        language="en";
    }
    if(self_test) {
        if(!run_world_self_test()) { std::cerr<<"World core test failed\n"; return 2; }
        if(!run_chunk_mesh_self_test()) { std::cerr<<"Chunk greedy-mesh test failed\n"; return 4; }
        if(!run_json_self_test() || !run_block_model_self_test()) { std::cerr<<"Resource model test failed\n"; return 3; }
        std::cout<<"World core, greedy chunk mesh and resource model tests passed\n";
        return 0;
    }
    if(open_3d || !export_map) return mc::render::run_terrain_viewer(size,seed);
    std::vector<double> heights(std::size_t(size)*size);
    for(int z=0;z<size;++z) for(int x=0;x<size;++x) heights[std::size_t(z)*size+x]=terrain(x,z,seed);
    std::ofstream out(output,std::ios::binary);
    if(!out) { std::cerr<<mc::i18n::text("error.output",language)<<output<<"\n"; return 1; }
    out<<"P6\n"<<size<<" "<<size<<"\n255\n";
    for(double h:heights) {
        unsigned char rgb[3];
        if(h<53) { rgb[0]=35; rgb[1]=75; rgb[2]=150; }
        else if(h<69) { rgb[0]=194; rgb[1]=178; rgb[2]=128; }
        else if(h<91) { rgb[0]=65; rgb[1]=145; rgb[2]=62; }
        else { rgb[0]=135; rgb[1]=135; rgb[2]=140; }
        out.write(reinterpret_cast<char*>(rgb),3);
    }
    std::string message=mc::i18n::text("terrain.generated",language);
    message=replace_all(message,"{size}",std::to_string(size));
    message=replace_all(message,"{seed}",std::to_string(seed));
    message=replace_all(message,"{output}",output);
    std::cout<<message<<"\n";
    return out ? 0 : 1;
}
