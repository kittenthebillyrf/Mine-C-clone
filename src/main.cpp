#include "i18n/Localization.hpp"
#include "world/World.hpp"
#include "world/generation/ChunkGenerator.hpp"
#include "render/TerrainViewer.hpp"

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
std::string replace_all(std::string value, const std::string& token, const std::string& replacement) {
    std::size_t pos=0;
    while((pos=value.find(token,pos))!=std::string::npos) {
        value.replace(pos,token.size(),replacement);
        pos += replacement.size();
    }
    return value;
}
bool run_world_self_test() {
    mc::world::World world;
    if(world.get_block(0,10,0)!=mc::world::BlockId::Air) return false;
    if(!world.set_block(0,10,0,mc::world::BlockId::Stone)) return false;
    if(!world.set_block(-1,20,-17,mc::world::BlockId::Grass)) return false;
    if(world.get_block(0,10,0)!=mc::world::BlockId::Stone) return false;
    if(world.get_block(-1,20,-17)!=mc::world::BlockId::Grass) return false;
    if(world.chunk_count()!=2) return false;
    if(world.set_block(0,256,0,mc::world::BlockId::Dirt)) return false;
    std::stringstream saved(std::ios::in|std::ios::out|std::ios::binary);
    if(!world.save(saved)) return false;
    mc::world::World restored;
    saved.seekg(0);
    if(!restored.load(saved)) return false;
    if(restored.chunk_count()!=2 ||
       restored.get_block(0,10,0)!=mc::world::BlockId::Stone ||
       restored.get_block(-1,20,-17)!=mc::world::BlockId::Grass) return false;
    if(!mc::world::block_info(mc::world::BlockId::Stone).solid) return false;

    const mc::world::generation::ChunkGenerator generator(1122);
    const auto column=generator.sample(0,0);
    if(column.height<5||column.height>205) return false;
    if(generator.sample(0,0).height!=column.height) return false;
    mc::world::World generated;
    generator.generate_chunk(generated,0,0);
    if(generated.get_block(0,0,0)!=mc::world::BlockId::Bedrock) return false;
    const auto surface=generated.get_block(0,column.height-1,0);
    if(surface!=mc::world::BlockId::Grass&&surface!=mc::world::BlockId::Sand) return false;
    return true;
}
}
int main(int argc,char** argv) {
    int size=128; std::uint32_t seed=1122;
    std::string output="terrain.ppm", language="en";
    bool self_test=false, open_3d=false, export_map=false;
    for(int i=1;i<argc;++i) {
        const std::string a=argv[i];
        if(a=="--size" && i+1<argc) size=std::clamp(std::atoi(argv[++i]),16,4096);
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
        std::cout<<"World core test passed\n";
        return 0;
    }
    if(open_3d || !export_map) return mc::render::run_terrain_viewer(size,seed);
    mc::world::generation::ChunkGenerator generator(seed);
    std::ofstream out(output,std::ios::binary);
    if(!out) { std::cerr<<mc::i18n::text("error.output",language)<<output<<"\n"; return 1; }
    out<<"P6\n"<<size<<" "<<size<<"\n255\n";
    for(int z=0;z<size;++z) for(int x=0;x<size;++x) {
        const auto column=generator.sample(x,z);
        unsigned char rgb[3];
        using mc::world::generation::Biome;
        if(column.height<62) { rgb[0]=35; rgb[1]=75; rgb[2]=150; }
        else if(column.biome==Biome::Desert||column.biome==Biome::Savanna) { rgb[0]=210; rgb[1]=190; rgb[2]=125; }
        else if(column.biome==Biome::Tundra||column.height>145) { rgb[0]=175; rgb[1]=180; rgb[2]=185; }
        else if(column.biome==Biome::Swamp) { rgb[0]=65; rgb[1]=105; rgb[2]=55; }
        else if(column.biome==Biome::Forest||column.biome==Biome::Taiga) { rgb[0]=45; rgb[1]=125; rgb[2]=58; }
        else { rgb[0]=90; rgb[1]=165; rgb[2]=70; }
        out.write(reinterpret_cast<char*>(rgb),3);
    }
    std::string message=mc::i18n::text("terrain.generated",language);
    message=replace_all(message,"{size}",std::to_string(size));
    message=replace_all(message,"{seed}",std::to_string(seed));
    message=replace_all(message,"{output}",output);
    std::cout<<message<<"\n";
    return out ? 0 : 1;
}
