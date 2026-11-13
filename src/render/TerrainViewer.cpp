#include "TerrainViewer.hpp"
#include "raylib.h"
#include "raymath.h"
#include "../world/World.hpp"
#include "../world/WorldGen112.hpp"
#include "../physics/PlayerPhysics.hpp"
#include "ChunkMesh.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#if defined(__SWITCH__)
#include <switch.h>
#include <sys/stat.h>
#endif

#if defined(__SWITCH__)
#define MCC_WORLD_SAVE_PATH "sdmc:/switch/MineCppClone/world.mcc"
#define MCC_PLAYER_SAVE_PATH "sdmc:/switch/MineCppClone/world.mcc.player"
#define MCC_BLOCK_ATLAS_PATH "romfs:/assets/blocks_atlas_1_12.png"
#else
#define MCC_WORLD_SAVE_PATH "world.mcc"
#define MCC_PLAYER_SAVE_PATH "world.mcc.player"
#define MCC_BLOCK_ATLAS_PATH "assets/blocks_atlas_1_12.png"
#endif

namespace mc::render {
namespace {
constexpr float BLOCK=2.0f;
constexpr float PLAYER_HEIGHT=1.8f;
constexpr float EYE_HEIGHT=1.62f;
constexpr float PLAYER_WIDTH=0.6f;
constexpr double PLAYER_STEP_HEIGHT=0.6;
constexpr float PLAYER_TICK=1.0f/20.0f;
constexpr float REACH=7.0f;
constexpr int HOTBAR_SIZE=9;
constexpr int INVENTORY_SIZE=36;
struct ItemStack { mc::world::BlockId id=mc::world::BlockId::Air; int count=0; };
struct DroppedItem { mc::world::BlockId id=mc::world::BlockId::Air; float x=0,y=0,z=0,velocityY=0; };
std::uint32_t hash2(int x,int z,std::uint32_t seed) {
    std::uint32_t h=seed^(std::uint32_t(x)*0x8da6b343u)^(std::uint32_t(z)*0xd8163841u);
    h^=h>>13; h*=0x85ebca6bu; h^=h>>16; h*=0xc2b2ae35u; h^=h>>16;
    return h;
}
float fade(float t) { return t*t*t*(t*(t*6.0f-15.0f)+10.0f); }
float lerp(float a,float b,float t) { return a+(b-a)*t; }
float lattice(int x,int z,std::uint32_t seed) {
    return (float(hash2(x,z,seed))/float(UINT32_MAX))*2.0f-1.0f;
}
float noise(float x,float z,std::uint32_t seed) {
    const int ix=int(std::floor(x)),iz=int(std::floor(z));
    const float fx=fade(x-float(ix)),fz=fade(z-float(iz));
    return lerp(lerp(lattice(ix,iz,seed),lattice(ix+1,iz,seed),fx),
                  lerp(lattice(ix,iz+1,seed),lattice(ix+1,iz+1,seed),fx),fz);
}
float fbm(float x,float z,std::uint32_t seed,int octaves,float frequency) {
    float v=0,a=1,total=0;
    for(int i=0;i<octaves;++i) { v+=noise(x*frequency,z*frequency,seed+std::uint32_t(i)*1013u)*a; total+=a; a*=0.5f; frequency*=2.0f; }
    return v/total;
}
int height_at(int x,int z,std::uint32_t seed) {
    const float broad=fbm(float(x),float(z),seed^0x9e3779b9u,4,0.035f);
    const float detail=fbm(float(x),float(z),seed,4,0.12f);
    return std::clamp(int(13.0f+broad*5.0f+detail*2.5f),5,23);
}
Color color_for(mc::world::BlockId id) {
    using mc::world::BlockId;
    switch(id) {
        case BlockId::Grass:return Color{82,160,65,255};
        case BlockId::Dirt:return Color{128,87,55,255};
        case BlockId::Stone:return Color{125,127,132,255};
        case BlockId::Sand:return Color{210,194,139,255};
        case BlockId::Water:return Color{55,115,205,170};
        case BlockId::Bedrock:return Color{55,55,58,255};
        case BlockId::Cobblestone:return Color{105,105,110,255};
        case BlockId::OakPlanks:return Color{170,130,75,255};
        case BlockId::OakLog:return Color{115,83,48,255};
        case BlockId::OakLeaves:return Color{65,130,55,255};
        case BlockId::Gravel:return Color{135,132,125,255};
        case BlockId::Sandstone:return Color{210,190,125,255};
        case BlockId::Glass:return Color{170,210,220,150};
        case BlockId::Bricks:return Color{150,75,65,255};
        case BlockId::Clay:return Color{155,160,165,255};
        case BlockId::CoalOre:return Color{75,75,75,255};
        case BlockId::IronOre:return Color{185,145,115,255};
        case BlockId::GoldOre:return Color{225,185,45,255};
        case BlockId::DiamondOre:return Color{65,210,195,255};
        case BlockId::RedstoneOre:return Color{175,45,40,255};
        case BlockId::LapisOre:return Color{45,75,170,255};
        case BlockId::EmeraldOre:return Color{45,175,90,255};
        case BlockId::Obsidian:return Color{45,35,65,255};
        case BlockId::Snow:return Color{235,242,250,255};
        case BlockId::Ice:return Color{155,205,235,190};
        case BlockId::Cactus:return Color{55,135,55,255};
        case BlockId::Bookshelf:return Color{155,110,65,255};
        case BlockId::WoolWhite:return Color{230,230,225,255};
        case BlockId::Netherrack:return Color{125,55,55,255};
        case BlockId::Glowstone:return Color{225,180,90,255};
        case BlockId::MossyCobblestone:return Color{90,115,85,255};
        case BlockId::Pumpkin:return Color{205,120,30,255};
        case BlockId::CraftingTable:return Color{145,95,55,255};
        case BlockId::Furnace:return Color{105,105,105,255};
        case BlockId::OakSlab:return Color{170,130,75,255};
        case BlockId::OakFence:return Color{150,110,60,255};
        case BlockId::Lava:return Color{235,85,20,255};
        case BlockId::SprucePlanks:return Color{105,76,45,255};
        case BlockId::BirchPlanks:return Color{195,174,116,255};
        case BlockId::JunglePlanks:return Color{160,111,78,255};
        case BlockId::AcaciaPlanks:return Color{165,86,55,255};
        case BlockId::DarkOakPlanks:return Color{65,45,27,255};
        case BlockId::SpruceLog:return Color{93,70,43,255};
        case BlockId::BirchLog:return Color{210,196,151,255};
        case BlockId::JungleLog:return Color{133,96,62,255};
        case BlockId::AcaciaLog:return Color{112,83,62,255};
        case BlockId::DarkOakLog:return Color{61,43,28,255};
        case BlockId::SpruceLeaves:return Color{45,91,61,255};
        case BlockId::BirchLeaves:return Color{91,150,62,255};
        case BlockId::JungleLeaves:return Color{53,133,48,255};
        case BlockId::AcaciaLeaves:return Color{76,139,57,255};
        case BlockId::DarkOakLeaves:return Color{48,101,39,255};
        case BlockId::Granite:return Color{151,108,96,255};
        case BlockId::PolishedGranite:return Color{167,125,111,255};
        case BlockId::Diorite:return Color{197,197,190,255};
        case BlockId::PolishedDiorite:return Color{211,211,207,255};
        case BlockId::Andesite:return Color{133,134,132,255};
        case BlockId::PolishedAndesite:return Color{145,146,145,255};
        case BlockId::StoneBricks:return Color{119,120,120,255};
        case BlockId::MossyStoneBricks:return Color{101,119,91,255};
        case BlockId::CrackedStoneBricks:return Color{111,111,110,255};
        case BlockId::ChiseledStoneBricks:return Color{124,124,122,255};
        case BlockId::Sponge:return Color{194,190,75,255};
        case BlockId::WetSponge:return Color{161,157,62,255};
        case BlockId::Prismarine:return Color{89,154,143,255};
        case BlockId::PrismarineBricks:return Color{100,171,154,255};
        case BlockId::DarkPrismarine:return Color{49,105,96,255};
        case BlockId::SeaLantern:return Color{171,218,207,255};
        case BlockId::EndStone:return Color{220,221,158,255};
        case BlockId::EndBricks:return Color{222,224,164,255};
        case BlockId::PurpurBlock:return Color{169,124,169,255};
        case BlockId::PurpurPillar:return Color{154,111,157,255};
        case BlockId::QuartzBlock:return Color{231,226,218,255};
        case BlockId::ChiseledQuartz:return Color{226,220,211,255};
        case BlockId::QuartzPillar:return Color{218,213,205,255};
        case BlockId::NetherQuartzOre:return Color{208,190,178,255};
        case BlockId::RedstoneLamp:return Color{171,104,70,255};
        case BlockId::HayBale:return Color{190,160,53,255};
        case BlockId::Melon:return Color{115,157,57,255};
        case BlockId::PumpkinLit:return Color{222,139,39,255};
        case BlockId::Terracotta:return Color{152,91,65,255};
        case BlockId::WhiteConcrete:return Color{207,213,214,255};
        case BlockId::OrangeConcrete:return Color{224,97,0,255};
        case BlockId::MagentaConcrete:return Color{169,48,159,255};
        case BlockId::LightBlueConcrete:return Color{35,137,198,255};
        case BlockId::YellowConcrete:return Color{241,175,21,255};
        case BlockId::LimeConcrete:return Color{94,168,24,255};
        case BlockId::PinkConcrete:return Color{214,101,143,255};
        case BlockId::GrayConcrete:return Color{54,57,62,255};
        case BlockId::LightGrayConcrete:return Color{125,125,115,255};
        case BlockId::CyanConcrete:return Color{21,119,136,255};
        case BlockId::PurpleConcrete:return Color{100,31,156,255};
        case BlockId::BlueConcrete:return Color{44,46,143,255};
        case BlockId::BrownConcrete:return Color{96,59,31,255};
        case BlockId::GreenConcrete:return Color{73,91,36,255};
        case BlockId::RedConcrete:return Color{142,32,32,255};
        case BlockId::BlackConcrete:return Color{8,10,15,255};
        default:return BLANK;
    }
}
mc::world::BlockId wood_planks_for(mc::world::BlockId id) {
    using mc::world::BlockId;
    switch(id) {
        case BlockId::OakSlab: case BlockId::OakFence: case BlockId::OakStairs: return BlockId::OakPlanks;
        case BlockId::SpruceSlab: case BlockId::SpruceFence: case BlockId::SpruceStairs: return BlockId::SprucePlanks;
        case BlockId::BirchSlab: case BlockId::BirchFence: case BlockId::BirchStairs: return BlockId::BirchPlanks;
        case BlockId::JungleSlab: case BlockId::JungleFence: case BlockId::JungleStairs: return BlockId::JunglePlanks;
        case BlockId::AcaciaSlab: case BlockId::AcaciaFence: case BlockId::AcaciaStairs: return BlockId::AcaciaPlanks;
        case BlockId::DarkOakSlab: case BlockId::DarkOakFence: case BlockId::DarkOakStairs: return BlockId::DarkOakPlanks;
        default: return id;
    }
}
bool is_wood_slab(mc::world::BlockId id) {
    using mc::world::BlockId;
    return id==BlockId::OakSlab || id==BlockId::SpruceSlab || id==BlockId::BirchSlab ||
           id==BlockId::JungleSlab || id==BlockId::AcaciaSlab || id==BlockId::DarkOakSlab;
}
bool is_wood_fence(mc::world::BlockId id) {
    using mc::world::BlockId;
    return id==BlockId::OakFence || id==BlockId::SpruceFence || id==BlockId::BirchFence ||
           id==BlockId::JungleFence || id==BlockId::AcaciaFence || id==BlockId::DarkOakFence;
}
bool is_wood_stairs(mc::world::BlockId id) {
    using mc::world::BlockId;
    return id==BlockId::OakStairs || id==BlockId::SpruceStairs || id==BlockId::BirchStairs ||
           id==BlockId::JungleStairs || id==BlockId::AcaciaStairs || id==BlockId::DarkOakStairs;
}
// BlockState facing follows Minecraft's horizontal order: north, east, south, west.
// The fallback is south so worlds created before directional states keep their old look.
int stair_facing(const mc::world::World& world,int x,int y,int z) {
    const auto* state=world.get_block_state(x,y,z);
    return state ? int(state->facing&3u) : 2;
}
int horizontal_facing_from_yaw(float yaw) {
    // Minecraft-style cardinal facing from the player's horizontal look angle.
    const float quarterTurns=yaw/(PI*0.5f);
    const int direction=int(std::lround(quarterTurns));
    return ((direction%4)+4)%4;
}
bool stair_upside_down(const mc::world::World& world,int x,int y,int z) {
    const auto* state=world.get_block_state(x,y,z);
    return state && (state->shape&1u)!=0;
}
Vector3 stair_upper_offset(int facing) {
    switch(facing&3) {
        case 0: return Vector3{0,0,-BLOCK*0.25f};
        case 1: return Vector3{BLOCK*0.25f,0,0};
        case 2: return Vector3{0,0,BLOCK*0.25f};
        default:return Vector3{-BLOCK*0.25f,0,0};
    }
}
int texture_slot(mc::world::BlockId id) {
    const int slot=static_cast<int>(wood_planks_for(id));
    return (slot>0 && slot<static_cast<int>(mc::world::BLOCK_ID_COUNT))?slot:-1;
}
int face_texture_slot(mc::world::BlockId id,Vector3 normal) {
    using mc::world::BlockId;
    const bool top=normal.y>0.5f, bottom=normal.y<-0.5f;
    switch(id) {
        case BlockId::Grass: return top?3:(bottom?2:98);
        case BlockId::OakLog: return (top||bottom)?99:9;
        case BlockId::Sandstone: return top?12:(bottom?101:100);
        case BlockId::Cactus: return top?102:(bottom?103:26);
        case BlockId::CraftingTable: return top?33:(bottom?8:(normal.z>0.5f?105:104));
        case BlockId::Furnace: return top?106:(normal.z>0.5f?107:34);
        case BlockId::Pumpkin: return (top||bottom)?108:32;
        case BlockId::PumpkinLit: return (top||bottom)?108:80;
        case BlockId::Melon: return (top||bottom)?109:79;
        case BlockId::HayBale: return (top||bottom)?110:78;
        default: return texture_slot(id);
    }
}
Model make_block_model(mc::world::BlockId id,Texture2D texture) {
    if(texture_slot(id)<0 || texture.id==0) return Model{};
    // Oak slabs are lower-half blocks in the default 1.12 state (half=bottom).
    const bool lowerSlab=is_wood_slab(id);
    // Vanilla cactus has a one-pixel inset on all four horizontal sides.
    const float horizontalSize=id==mc::world::BlockId::Cactus?BLOCK*0.875f:BLOCK;
    Mesh mesh=GenMeshCube(horizontalSize,lowerSlab?BLOCK*0.5f:BLOCK,horizontalSize);
    for(int vertex=0;vertex<mesh.vertexCount;++vertex) {
        const Vector3 normal={mesh.normals[vertex*3],mesh.normals[vertex*3+1],mesh.normals[vertex*3+2]};
        const int slot=face_texture_slot(id,normal);
        if(slot<0) continue;
        const float oldU=mesh.texcoords[vertex*2],oldV=mesh.texcoords[vertex*2+1];
        const float col=float(slot%16),row=float(slot/16);
        mesh.texcoords[vertex*2]=(col+oldU)/16.0f;
        mesh.texcoords[vertex*2+1]=(row+oldV)/8.0f;
    }
    Model model=LoadModelFromMesh(mesh);
    if(model.materialCount>0)
        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=texture;
    return model;
}

Model make_chunk_model(const ChunkMeshData& data,Texture2D texture) {
    if(data.empty() || texture.id==0) return Model{};
    Mesh mesh{};
    mesh.vertexCount=static_cast<int>(data.vertices.size());
    mesh.triangleCount=static_cast<int>(data.indices.size()/3);
    mesh.vertices=static_cast<float*>(MemAlloc(sizeof(float)*data.vertices.size()*3));
    mesh.normals=static_cast<float*>(MemAlloc(sizeof(float)*data.vertices.size()*3));
    mesh.texcoords=static_cast<float*>(MemAlloc(sizeof(float)*data.vertices.size()*2));
    mesh.indices=static_cast<unsigned short*>(MemAlloc(sizeof(unsigned short)*data.indices.size()));
    if(!mesh.vertices||!mesh.normals||!mesh.texcoords||!mesh.indices) {
        if(mesh.vertices)MemFree(mesh.vertices);if(mesh.normals)MemFree(mesh.normals);
        if(mesh.texcoords)MemFree(mesh.texcoords);if(mesh.indices)MemFree(mesh.indices);
        return Model{};
    }
    for(std::size_t face=0;face+3<data.vertices.size();face+=4) {
        const auto& first=data.vertices[face];
        const Vector3 normal{first.nx,first.ny,first.nz};
        const int slot=std::clamp(face_texture_slot(static_cast<mc::world::BlockId>(first.block_id),normal),0,127);
        const float tileX=float(slot%16),tileY=float(slot/16);
        float minU=1e30f,maxU=-1e30f,minV=1e30f,maxV=-1e30f;
        auto raw_uv=[&](const ChunkMeshVertex& vertex) {
            if(std::abs(normal.x)>0.5f) return Vector2{vertex.z,vertex.y};
            if(std::abs(normal.y)>0.5f) return Vector2{vertex.x,vertex.z};
            return Vector2{vertex.x,vertex.y};
        };
        for(std::size_t k=0;k<4;++k) {
            const Vector2 uv=raw_uv(data.vertices[face+k]);
            minU=std::min(minU,uv.x);maxU=std::max(maxU,uv.x);
            minV=std::min(minV,uv.y);maxV=std::max(maxV,uv.y);
        }
        const float du=std::max(1e-6f,maxU-minU),dv=std::max(1e-6f,maxV-minV);
        for(std::size_t k=0;k<4;++k) {
            const std::size_t index=face+k;
            const auto& vertex=data.vertices[index];
            mesh.vertices[index*3+0]=vertex.x*BLOCK;
            mesh.vertices[index*3+1]=vertex.y*BLOCK;
            mesh.vertices[index*3+2]=vertex.z*BLOCK;
            mesh.normals[index*3+0]=vertex.nx;
            mesh.normals[index*3+1]=vertex.ny;
            mesh.normals[index*3+2]=vertex.nz;
            const Vector2 uv=raw_uv(vertex);
            const float localU=(uv.x-minU)/du,localV=(uv.y-minV)/dv;
            mesh.texcoords[index*2+0]=(tileX+0.001f+localU*0.998f)/16.0f;
            mesh.texcoords[index*2+1]=(tileY+0.001f+localV*0.998f)/8.0f;
        }
    }
    std::copy(data.indices.begin(),data.indices.end(),mesh.indices);
    UploadMesh(&mesh,false);
    Model model=LoadModelFromMesh(mesh);
    if(model.materialCount>0) model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=texture;
    return model;
}
bool is_chunk_mesh_block(mc::world::BlockId id) {
    return ChunkMeshBuilder::is_full_cube_block(id);
}
struct RuntimeChunkMesh {
    int x=0,z=0,section=0;
    ChunkMeshData data;
    Model model{};
};
bool section_in_frustum(const Camera3D& camera,int cx,int cz,int section,int screenWidth,int screenHeight) {
    const Vector3 forward=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
    const Vector3 right=Vector3Normalize(Vector3CrossProduct(forward,camera.up));
    const Vector3 up=Vector3Normalize(Vector3CrossProduct(right,forward));
    const Vector3 center{(float(cx*mc::world::Chunk::Width)+8.0f)*BLOCK,
                         (float(section*ChunkMeshBuilder::SectionHeight)+8.0f)*BLOCK,
                         (float(cz*mc::world::Chunk::Depth)+8.0f)*BLOCK};
    const Vector3 delta=Vector3Subtract(center,camera.position);
    constexpr float radius=27.72f; // Bounding sphere around a 16x16x16 section.
    const float depth=Vector3DotProduct(delta,forward);
    if(depth+radius<0.1f) return false;
    const float safeDepth=std::max(0.1f,depth);
    const float halfHeight=std::tan(camera.fovy*DEG2RAD*0.5f)*safeDepth;
    const float halfWidth=halfHeight*(float(screenWidth)/float(std::max(1,screenHeight)));
    if(std::abs(Vector3DotProduct(delta,right))>halfWidth+radius) return false;
    if(std::abs(Vector3DotProduct(delta,up))>halfHeight+radius) return false;
    return true;
}
bool fence_connects_to(mc::world::BlockId id) {
    using mc::world::BlockId;
    if(id==BlockId::Air || id==BlockId::Water || id==BlockId::Lava ||
       id==BlockId::Glass || id==BlockId::Ice || id==BlockId::Torch ||
       id==BlockId::Cactus || is_wood_slab(id)) return false;
    return mc::world::block_info(id).solid || is_wood_fence(id);
}
void draw_block(Model* models,const mc::world::World& world,mc::world::BlockId id,
                int blockX,int blockY,int blockZ,Vector3 center) {
    const auto index=static_cast<unsigned int>(id);
    if(id==mc::world::BlockId::Torch) {
        // Compact upright torch: dark wooden shaft, warm collar, animated-looking flame.
        const Vector3 base={center.x,center.y-BLOCK*0.34f,center.z};
        const Vector3 tip={center.x,center.y+BLOCK*0.12f,center.z};
        DrawCylinderEx(base,tip,0.075f,0.055f,6,Color{105,67,35,255});
        const Vector3 flameBase={center.x,center.y+BLOCK*0.10f,center.z};
        const Vector3 flameTip={center.x+0.025f,center.y+BLOCK*0.34f,center.z};
        DrawCylinderEx(flameBase,flameTip,0.105f,0.015f,6,Color{255,151,35,255});
        DrawSphere(Vector3{center.x,center.y+BLOCK*0.16f,center.z},0.075f,Color{255,220,105,245});
        // Soft local glow cue; actual world-light propagation is a later lighting stage.
        DrawSphere(Vector3{center.x,center.y+BLOCK*0.12f,center.z},0.22f,Color{255,170,55,28});
        return;
    }
    if(is_wood_stairs(id) && models[static_cast<unsigned int>(wood_planks_for(id))].meshCount>0) {
        const Model& plank=models[static_cast<unsigned int>(wood_planks_for(id))];
        const int facing=stair_facing(world,blockX,blockY,blockZ);
        const bool upsideDown=stair_upside_down(world,blockX,blockY,blockZ);
        const Vector3 upperOffset=stair_upper_offset(facing);
        const float fullY=upsideDown?BLOCK*0.25f:-BLOCK*0.25f;
        const float halfY=upsideDown?-BLOCK*0.25f:BLOCK*0.25f;
        DrawModelEx(plank,Vector3{center.x,center.y+fullY,center.z},
                    Vector3{0,1,0},0.0f,Vector3{1.0f,0.5f,1.0f},WHITE);
        const Vector3 halfScale=(facing==0 || facing==2)
            ? Vector3{1.0f,0.5f,0.5f} : Vector3{0.5f,0.5f,1.0f};
        DrawModelEx(plank,Vector3{center.x+upperOffset.x,center.y+halfY,center.z+upperOffset.z},
                    Vector3{0,1,0},0.0f,halfScale,WHITE);
        return;
    }
    if(id==mc::world::BlockId::Water || id==mc::world::BlockId::Glass || id==mc::world::BlockId::Ice || id==mc::world::BlockId::Lava) {
        DrawCube(center,BLOCK,BLOCK,BLOCK,color_for(id)); return;
    }
    if(index<mc::world::BLOCK_ID_COUNT && models[index].meshCount>0) {
        if(is_wood_fence(id)) {
            // Default connected-fence silhouette; all pieces use this wood species' plank texture.
            const Model& fence=models[static_cast<unsigned int>(wood_planks_for(id))];
            DrawModelEx(fence,center,Vector3{0,1,0},0.0f,Vector3{0.25f,1.0f,0.25f},WHITE);
            const bool north=fence_connects_to(world.get_block(blockX,blockY,blockZ-1));
            const bool south=fence_connects_to(world.get_block(blockX,blockY,blockZ+1));
            const bool west=fence_connects_to(world.get_block(blockX-1,blockY,blockZ));
            const bool east=fence_connects_to(world.get_block(blockX+1,blockY,blockZ));
            for(float y : {-0.35f,0.35f}) {
                if(west || east)
                    DrawModelEx(fence,Vector3{center.x,center.y+y,center.z},Vector3{0,1,0},0.0f,
                                Vector3{1.0f,0.1875f,0.1875f},WHITE);
                if(north || south)
                    DrawModelEx(fence,Vector3{center.x,center.y+y,center.z},Vector3{0,1,0},0.0f,
                                Vector3{0.1875f,0.1875f,1.0f},WHITE);
            }
            return;
        }
        Vector3 modelCenter=center;
        if(is_wood_slab(id)) modelCenter.y-=BLOCK*0.25f;
        DrawModel(models[index],modelCenter,1.0f,WHITE);
    } else DrawCube(center,BLOCK,BLOCK,BLOCK,color_for(id));
}
Vector3 block_center(int x,int y,int z) { return Vector3{(float(x)+0.5f)*BLOCK,(float(y)+0.5f)*BLOCK,(float(z)+0.5f)*BLOCK}; }
struct PlayerAabb {
    double minX,minY,minZ,maxX,maxY,maxZ;
    PlayerAabb offset(double x,double y,double z) const {
        return {minX+x,minY+y,minZ+z,maxX+x,maxY+y,maxZ+z};
    }
    PlayerAabb expand(double x,double y,double z) const {
        PlayerAabb out=*this;
        if(x<0.0) out.minX+=x; else out.maxX+=x;
        if(y<0.0) out.minY+=y; else out.maxY+=y;
        if(z<0.0) out.minZ+=z; else out.maxZ+=z;
        return out;
    }
};
PlayerAabb player_box(double x,double feetY,double z) {
    constexpr double half=double(PLAYER_WIDTH)*0.5;
    return {x-half,feetY,z-half,x+half,feetY+PLAYER_HEIGHT,z+half};
}
bool intersects(const PlayerAabb& a,const PlayerAabb& b) {
    return a.maxX>b.minX && a.minX<b.maxX &&
           a.maxY>b.minY && a.minY<b.maxY &&
           a.maxZ>b.minZ && a.minZ<b.maxZ;
}
bool is_solid(mc::world::BlockId id) {
    return id!=mc::world::BlockId::Air && mc::world::block_info(id).solid;
}
std::vector<PlayerAabb> collision_boxes(const mc::world::World& world,const PlayerAabb& query) {
    std::vector<PlayerAabb> boxes;
    const int minX=int(std::floor(query.minX)),maxX=int(std::floor(query.maxX));
    const int minY=std::max(0,int(std::floor(query.minY)));
    const int maxY=std::min(mc::world::Chunk::Height-1,int(std::floor(query.maxY)));
    const int minZ=int(std::floor(query.minZ)),maxZ=int(std::floor(query.maxZ));
    for(int y=minY;y<=maxY;++y)
        for(int z=minZ;z<=maxZ;++z)
            for(int x=minX;x<=maxX;++x) {
                const auto id=world.get_block(x,y,z);
                if(!is_solid(id)) continue;
                if(is_wood_slab(id)) {
                    const PlayerAabb slab{double(x),double(y),double(z),
                                          double(x+1),double(y)+0.5,double(z+1)};
                    if(intersects(slab,query)) boxes.push_back(slab);
                    continue;
                }
                if(is_wood_stairs(id)) {
                    const int facing=stair_facing(world,x,y,z);
                    const bool upsideDown=stair_upside_down(world,x,y,z);
                    const double lowerY=upsideDown?double(y)+0.5:double(y);
                    const double upperY=upsideDown?double(y):double(y)+0.5;
                    const PlayerAabb full{double(x),lowerY,double(z),
                                          double(x+1),lowerY+0.5,double(z+1)};
                    PlayerAabb half{double(x),upperY,double(z),double(x+1),upperY+0.5,double(z+1)};
                    if(facing==0) half.maxZ=double(z)+0.5;
                    else if(facing==1) half.minX=double(x)+0.5;
                    else if(facing==2) half.minZ=double(z)+0.5;
                    else half.maxX=double(x)+0.5;
                    if(intersects(full,query)) boxes.push_back(full);
                    if(intersects(half,query)) boxes.push_back(half);
                    continue;
                }
                if(is_wood_fence(id)) {
                    const auto add=[&](PlayerAabb shape) {
                        if(intersects(shape,query)) boxes.push_back(shape);
                    };
                    // Fence posts are 1.5 blocks tall and one quarter-block wide.
                    add({double(x)+0.375,double(y),double(z)+0.375,
                         double(x)+0.625,double(y)+1.5,double(z)+0.625});
                    const bool north=fence_connects_to(world.get_block(x,y,z-1));
                    const bool south=fence_connects_to(world.get_block(x,y,z+1));
                    const bool west=fence_connects_to(world.get_block(x-1,y,z));
                    const bool east=fence_connects_to(world.get_block(x+1,y,z));
                    for(const auto [low,high] : {std::pair{0.375,0.9375},std::pair{1.0,1.5}}) {
                        if(west) add({double(x),double(y)+low,double(z)+0.375,
                                      double(x)+0.5,double(y)+high,double(z)+0.625});
                        if(east) add({double(x)+0.5,double(y)+low,double(z)+0.375,
                                      double(x+1),double(y)+high,double(z)+0.625});
                        if(north) add({double(x)+0.375,double(y)+low,double(z),
                                       double(x)+0.625,double(y)+high,double(z)+0.5});
                        if(south) add({double(x)+0.375,double(y)+low,double(z)+0.5,
                                       double(x)+0.625,double(y)+high,double(z+1)});
                    }
                    continue;
                }
                const PlayerAabb block{double(x),double(y),double(z),double(x+1),double(y+1),double(z+1)};
                if(intersects(block,query)) boxes.push_back(block);
            }
    return boxes;
}
double clip_y(const PlayerAabb& obstacle,const PlayerAabb& moving,double offset) {
    if(obstacle.maxX>moving.minX && obstacle.minX<moving.maxX &&
       obstacle.maxZ>moving.minZ && obstacle.minZ<moving.maxZ) {
        if(offset>0.0 && moving.maxY<=obstacle.minY)
            offset=std::min(offset,obstacle.minY-moving.maxY);
        else if(offset<0.0 && moving.minY>=obstacle.maxY)
            offset=std::max(offset,obstacle.maxY-moving.minY);
    }
    return offset;
}
double clip_x(const PlayerAabb& obstacle,const PlayerAabb& moving,double offset) {
    if(obstacle.maxY>moving.minY && obstacle.minY<moving.maxY &&
       obstacle.maxZ>moving.minZ && obstacle.minZ<moving.maxZ) {
        if(offset>0.0 && moving.maxX<=obstacle.minX)
            offset=std::min(offset,obstacle.minX-moving.maxX);
        else if(offset<0.0 && moving.minX>=obstacle.maxX)
            offset=std::max(offset,obstacle.maxX-moving.minX);
    }
    return offset;
}
double clip_z(const PlayerAabb& obstacle,const PlayerAabb& moving,double offset) {
    if(obstacle.maxX>moving.minX && obstacle.minX<moving.maxX &&
       obstacle.maxY>moving.minY && obstacle.minY<moving.maxY) {
        if(offset>0.0 && moving.maxZ<=obstacle.minZ)
            offset=std::min(offset,obstacle.minZ-moving.maxZ);
        else if(offset<0.0 && moving.minZ>=obstacle.maxZ)
            offset=std::max(offset,obstacle.maxZ-moving.minZ);
    }
    return offset;
}
bool overlaps_liquid(const mc::world::World& world,const PlayerAabb& query) {
    const int minX=int(std::floor(query.minX)),maxX=int(std::floor(query.maxX));
    const int minY=std::max(0,int(std::floor(query.minY)));
    const int maxY=std::min(mc::world::Chunk::Height-1,int(std::floor(query.maxY)));
    const int minZ=int(std::floor(query.minZ)),maxZ=int(std::floor(query.maxZ));
    for(int y=minY;y<=maxY;++y) for(int z=minZ;z<=maxZ;++z) for(int x=minX;x<=maxX;++x) {
        const auto id=world.get_block(x,y,z);
        if(id==mc::world::BlockId::Water || id==mc::world::BlockId::Lava) {
            const PlayerAabb liquid{double(x),double(y),double(z),double(x+1),double(y+1),double(z+1)};
            if(intersects(liquid,query)) return true;
        }
    }
    return false;
}
bool in_water(const mc::world::World& world,double x,double y,double z) {
    const PlayerAabb b=player_box(x,y,z);
    return overlaps_liquid(world,{b.minX+0.001,b.minY+0.401,b.minZ+0.001,
                                  b.maxX-0.001,b.maxY-0.401,b.maxZ-0.001}) &&
           [&]() {
               const PlayerAabb q{b.minX+0.001,b.minY+0.401,b.minZ+0.001,
                                  b.maxX-0.001,b.maxY-0.401,b.maxZ-0.001};
               for(int by=std::max(0,int(std::floor(q.minY)));by<=std::min(mc::world::Chunk::Height-1,int(std::floor(q.maxY)));++by)
                   for(int bz=int(std::floor(q.minZ));bz<=int(std::floor(q.maxZ));++bz)
                       for(int bx=int(std::floor(q.minX));bx<=int(std::floor(q.maxX));++bx)
                           if(world.get_block(bx,by,bz)==mc::world::BlockId::Water &&
                              intersects({double(bx),double(by),double(bz),double(bx+1),double(by+1),double(bz+1)},q)) return true;
               return false;
           }();
}
bool in_lava(const mc::world::World& world,double x,double y,double z) {
    const PlayerAabb b=player_box(x,y,z);
    const PlayerAabb q{b.minX+0.1,b.minY+0.4,b.minZ+0.1,b.maxX-0.1,b.maxY-0.4,b.maxZ-0.1};
    const int minX=int(std::floor(q.minX)),maxX=int(std::floor(q.maxX));
    const int minY=std::max(0,int(std::floor(q.minY))),maxY=std::min(mc::world::Chunk::Height-1,int(std::floor(q.maxY)));
    const int minZ=int(std::floor(q.minZ)),maxZ=int(std::floor(q.maxZ));
    for(int by=minY;by<=maxY;++by) for(int bz=minZ;bz<=maxZ;++bz) for(int bx=minX;bx<=maxX;++bx)
        if(world.get_block(bx,by,bz)==mc::world::BlockId::Lava &&
           intersects({double(bx),double(by),double(bz),double(bx+1),double(by+1),double(bz+1)},q)) return true;
    return false;
}
bool offset_position_is_liquid(const mc::world::World& world,double x,double y,double z,const PlayerAabb& box) {
    const PlayerAabb moved=box.offset(x,y,z);
    return collision_boxes(world,moved).empty() && !overlaps_liquid(world,moved);
}
bool move_player_entity(const mc::world::World& world,double& px,double& py,double& pz,
                        double& dx,double& dy,double& dz,bool sneaking,bool wasOnGround,
                        bool& collidedHorizontally,bool& collidedVertically,bool& onGround) {
    return mc::physics::move_player(world,px,py,pz,dx,dy,dz,sneaking,wasOnGround,
                                    collidedHorizontally,collidedVertically,onGround);
}
void move_relative(double& motionX,double& motionY,double& motionZ,
                   float strafe,float up,float forward,float friction,float yaw) {
    float lengthSq=strafe*strafe+up*up+forward*forward;
    if(lengthSq>=1.0e-4F) {
        float length=std::sqrt(lengthSq);
        if(length<1.0F) length=1.0F;
        const float scale=friction/length;
        strafe*=scale; up*=scale; forward*=scale;
        const float sinYaw=std::sin(yaw*0.017453292F);
        const float cosYaw=std::cos(yaw*0.017453292F);
        motionX+=double(strafe*cosYaw-forward*sinYaw);
        motionY+=double(up);
        motionZ+=double(forward*cosYaw+strafe*sinYaw);
    }
}
float block_slipperiness(mc::world::BlockId id) {
    return id==mc::world::BlockId::Ice ? 0.98F : 0.6F;
}
void tick_vanilla_player(const mc::world::World& world,double& px,double& py,double& pz,
                         double& motionX,double& motionY,double& motionZ,bool& onGround,
                         bool& collidedHorizontally,int& jumpTicks,float moveStrafe,float moveForward,
                         bool jumping,bool sneaking,bool sprinting,float cameraYaw) {
    if(jumpTicks>0) --jumpTicks;
    // EntityLivingBase.onLivingUpdate(): client-side inertial damping and tiny-motion cutoff.
    motionX*=0.98; motionY*=0.98; motionZ*=0.98;
    if(std::abs(motionX)<0.003) motionX=0.0;
    if(std::abs(motionY)<0.003) motionY=0.0;
    if(std::abs(motionZ)<0.003) motionZ=0.0;

    const bool water=in_water(world,px,py,pz);
    const bool lava=in_lava(world,px,py,pz);
    if(!jumping) jumpTicks=0;
    else if(water || lava) motionY+=0.03999999910593033;
    else if(onGround && jumpTicks==0) {
        // EntityLivingBase.jump(): exact 1.12.2 base impulse and sprint impulse.
        motionY=0.42;
        if(sprinting) {
            const float mcYawRadians=cameraYaw+3.14159265358979323846F;
            motionX-=double(std::sin(mcYawRadians)*0.2F);
            motionZ+=double(std::cos(mcYawRadians)*0.2F);
        }
        jumpTicks=10;
    }

    moveStrafe*=0.98F;
    moveForward*=0.98F;
    const float mcYawDegrees=cameraYaw*57.29577951308232F+180.0F;
    if(water) {
        const double oldY=py;
        move_relative(motionX,motionY,motionZ,moveStrafe,0.0F,moveForward,0.02F,mcYawDegrees);
        bool cH=false,cV=false;
        move_player_entity(world,px,py,pz,motionX,motionY,motionZ,sneaking,onGround,cH,cV,onGround);
        if(cH) motionX=motionZ=0.0;
        if(cV) motionY=0.0;
        motionX*=0.8; motionY*=0.800000011920929; motionZ*=0.8;
        motionY-=0.02;
        if(cH && offset_position_is_liquid(world,motionX,motionY+0.6000000238418579-py+oldY,motionZ,player_box(px,py,pz)))
            motionY=0.30000001192092896;
        collidedHorizontally=cH;
    } else if(lava) {
        const double oldY=py;
        move_relative(motionX,motionY,motionZ,moveStrafe,0.0F,moveForward,0.02F,mcYawDegrees);
        bool cH=false,cV=false;
        move_player_entity(world,px,py,pz,motionX,motionY,motionZ,sneaking,onGround,cH,cV,onGround);
        if(cH) motionX=motionZ=0.0;
        if(cV) motionY=0.0;
        motionX*=0.5; motionY*=0.5; motionZ*=0.5;
        motionY-=0.02;
        if(cH && offset_position_is_liquid(world,motionX,motionY+0.6000000238418579-py+oldY,motionZ,player_box(px,py,pz)))
            motionY=0.30000001192092896;
        collidedHorizontally=cH;
    } else {
        const int belowX=int(std::floor(px)),belowY=int(std::floor(py-1.0)),belowZ=int(std::floor(pz));
        float friction=0.91F;
        if(onGround) friction=block_slipperiness(world.get_block(belowX,belowY,belowZ))*0.91F;
        const float acceleration=0.16277136F/(friction*friction*friction);
        const float moveFactor=onGround?(sprinting?0.13000001F:0.1F)*acceleration:(sprinting?0.026F:0.02F);
        move_relative(motionX,motionY,motionZ,moveStrafe,0.0F,moveForward,moveFactor,mcYawDegrees);
        friction=0.91F;
        if(onGround) friction=block_slipperiness(world.get_block(belowX,belowY,belowZ))*0.91F;
        bool cH=false,cV=false;
        move_player_entity(world,px,py,pz,motionX,motionY,motionZ,sneaking,onGround,cH,cV,onGround);
        if(cH) motionX=motionZ=0.0;
        if(cV) motionY=0.0;
        motionY-=0.08;
        motionY*=0.9800000190734863;
        motionX*=double(friction);
        motionZ*=double(friction);
        collidedHorizontally=cH;
    }
}
int surface_y(const mc::world::World& world,int x,int z) {
    for(int y=mc::world::Chunk::Height-1;y>=0;--y) {
        const auto id=world.get_block(x,y,z);
        if(id!=mc::world::BlockId::Air&&id!=mc::world::BlockId::Water) return y+1;
    }
    return 1;
}
bool replace_saved_file(const std::string& temporary,const std::string& destination) {
    const std::string backup=destination+".bak";
    std::remove(backup.c_str());
    errno=0;
    const bool hadPrevious=(std::rename(destination.c_str(),backup.c_str())==0);
    if(!hadPrevious && errno!=ENOENT) return false;
    if(std::rename(temporary.c_str(),destination.c_str())!=0) {
        if(hadPrevious) std::rename(backup.c_str(),destination.c_str());
        return false;
    }
    if(hadPrevious) std::remove(backup.c_str());
    return true;
}
bool save_world_file(const mc::world::World& world,
                     double px,double py,double pz,double motionX,double motionY,double motionZ,
                     float yaw,float pitch,const ItemStack* inventory,int selected) {
    const std::string worldPath=MCC_WORLD_SAVE_PATH;
    const std::string worldTemp=worldPath+".tmp";
    {
        std::ofstream output(worldTemp,std::ios::binary|std::ios::trunc);
        if(!output || !world.save(output)) return false;
        output.flush();
        if(!output) return false;
    }
    // Save player metadata separately from the block-world payload. This is
    // our own format; no Java Anvil/NBT files are read or written.
    const std::string playerPath=MCC_PLAYER_SAVE_PATH;
    const std::string playerTemp=playerPath+".tmp";
    {
        std::ofstream output(playerTemp,std::ios::trunc);
        if(!output) return false;
        output << "MCCPLAYER 1\n" << std::setprecision(17)
               << px << ' ' << py << ' ' << pz << ' '
               << motionX << ' ' << motionY << ' ' << motionZ << ' '
               << yaw << ' ' << pitch << ' ' << selected << '\n';
        for(int i=0;i<INVENTORY_SIZE;++i)
            output << static_cast<unsigned int>(inventory[i].id) << ' '
                   << inventory[i].count << '\n';
        output.flush();
        if(!output) return false;
    }
    if(!replace_saved_file(worldTemp,worldPath)) return false;
    if(!replace_saved_file(playerTemp,playerPath)) return false;
    return true;
}
bool load_player_state(double& px,double& py,double& pz,
                       double& motionX,double& motionY,double& motionZ,
                       float& yaw,float& pitch,ItemStack* inventory,int& selected) {
    std::ifstream input(MCC_PLAYER_SAVE_PATH);
    if(!input) return false;
    std::string magic;
    int version=0;
    if(!(input>>magic>>version)||magic!="MCCPLAYER"||version!=1) return false;
    double x=0,y=0,z=0,vx=0,vy=0,vz=0;
    float loadedYaw=0,loadedPitch=0;
    int hotbar=0;
    if(!(input>>x>>y>>z>>vx>>vy>>vz>>loadedYaw>>loadedPitch>>hotbar)) return false;
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||
       !std::isfinite(vx)||!std::isfinite(vy)||!std::isfinite(vz)||
       !std::isfinite(loadedYaw)||!std::isfinite(loadedPitch)||
       std::abs(x)>30000000.0||std::abs(z)>30000000.0||
       y<0.0||y>double(mc::world::Chunk::Height+8)||hotbar<0||hotbar>=HOTBAR_SIZE) return false;
    ItemStack loaded[INVENTORY_SIZE]{};
    for(int i=0;i<INVENTORY_SIZE;++i) {
        unsigned int id=0;
        int count=0;
        if(!(input>>id>>count)||id>=mc::world::BLOCK_ID_COUNT||count<0||count>64) return false;
        loaded[i]={static_cast<mc::world::BlockId>(id),count};
        if(count==0) loaded[i].id=mc::world::BlockId::Air;
    }
    std::string trailing;
    if(input>>trailing) return false;
    px=x;py=y;pz=z;motionX=vx;motionY=vy;motionZ=vz;
    yaw=loadedYaw;pitch=std::clamp(loadedPitch,-1.55f,1.55f);
    selected=hotbar;
    std::copy_n(loaded,INVENTORY_SIZE,inventory);
    return true;
}
bool add_item(ItemStack* inventory, mc::world::BlockId id) {
    // Merge into existing stacks before using an empty slot.
    for(int i=0;i<INVENTORY_SIZE;++i) {
        if(inventory[i].id==id && inventory[i].count>0 && inventory[i].count<64) {
            ++inventory[i].count;
            return true;
        }
    }
    for(int i=0;i<INVENTORY_SIZE;++i) {
        if(inventory[i].count==0) {
            inventory[i]={id,1};
            return true;
        }
    }
    return false;
}
bool ray_block(Ray ray,mc::world::World& world,Vector3& hit,Vector3& previous) {
    Vector3 p=ray.position;
    const Vector3 step=Vector3Scale(Vector3Normalize(ray.direction),0.08f);
    int lastX=0,lastY=0,lastZ=0;
    for(float d=0;d<REACH;d+=0.08f) {
        const int x=int(std::floor(p.x/BLOCK)),y=int(std::floor(p.y/BLOCK)),z=int(std::floor(p.z/BLOCK));
        const auto id=world.get_block(x,y,z);
        if(id!=mc::world::BlockId::Air && id!=mc::world::BlockId::Water) {
            hit=Vector3{float(x),float(y),float(z)};
            previous=Vector3{float(lastX),float(lastY),float(lastZ)};
            return true;
        }
        lastX=x; lastY=y; lastZ=z; p=Vector3Add(p,step);
    }
    return false;
}
}
int run_terrain_viewer(int map_size,std::uint32_t seed) {
    using namespace mc::world;
    const int size=std::clamp(map_size,32,96);
    World world;
    OverworldGenerator112 overworld(static_cast<std::int64_t>(seed));
    const int chunks=(size+15)/16;
    for(int cz=0;cz<chunks;++cz) for(int cx=0;cx<chunks;++cx)
        overworld.generate_chunk(world,cx,cz);

    #if defined(__SWITCH__)
    mkdir("sdmc:/switch/MineCppClone",0777);
#endif
    { std::ifstream saveFile(MCC_WORLD_SAVE_PATH,std::ios::binary); if(saveFile) world.load(saveFile); }

    InitWindow(1280,720,"Mine C++ Clone - playable voxel prototype");
    SetTargetFPS(60);
    DisableCursor();
    Model blockModels[BLOCK_ID_COUNT]{};
    std::vector<RuntimeChunkMesh> runtimeChunkMeshes;
    runtimeChunkMeshes.reserve(96);
#if defined(__SWITCH__)
    // NROs do not expose romfs paths until the filesystem is mounted.
    romfsInit();
#endif
    Image blockAtlas=LoadImage(MCC_BLOCK_ATLAS_PATH);
#if !defined(__SWITCH__)
    if(!IsImageValid(blockAtlas)) blockAtlas=LoadImage("../assets/blocks_atlas_1_12.png");
#endif
    Texture2D blockTexture={0};
    if(IsImageValid(blockAtlas)) {
        blockTexture=LoadTextureFromImage(blockAtlas);
        if(blockTexture.id!=0) SetTextureFilter(blockTexture,TEXTURE_FILTER_POINT);
        for(unsigned int raw=1;raw<BLOCK_ID_COUNT;++raw) {
            const BlockId id=static_cast<BlockId>(raw);
            blockModels[raw]=make_block_model(id,blockTexture);
        }
        UnloadImage(blockAtlas);
    }
    double px=double(size)*0.5,pz=double(size)*0.5;
    double py=double(surface_y(world,int(px),int(pz)));
    double motionX=0.0,motionY=0.0,motionZ=0.0;
    float yaw=0.0f,pitch=-0.18f;
    bool grounded=true;
    bool collidedHorizontally=false;
    bool sprinting=false;
    bool previousForward=false;
    int jumpTicks=0,sprintToggleTimer=0;
    float physicsAccumulator=0.0f;
    float fallingBlockAccumulator=0.0f;
    int selected=0;
    int creativePage=0;
    ItemStack inventory[INVENTORY_SIZE]{};
    std::vector<DroppedItem> droppedItems;
    bool creativeMode=false;
    const BlockId startingItems[HOTBAR_SIZE]={
        BlockId::Grass,BlockId::Dirt,BlockId::Stone,BlockId::Cobblestone,
        BlockId::OakPlanks,BlockId::OakLog,BlockId::Sand,BlockId::Glass,BlockId::Water
    };
    for(int i=0;i<HOTBAR_SIZE;++i) inventory[i]={startingItems[i],32};
    // Restore the complete local player session when a matching custom save
    // exists. Missing/invalid metadata leaves a fresh spawn and starter bar.
    const bool playerLoaded=load_player_state(px,py,pz,motionX,motionY,motionZ,yaw,pitch,inventory,selected);
    // New games (and old saves that resume underwater) choose the nearest dry,
    // walkable surface instead of placing the player on the ocean floor.
    if(!playerLoaded || in_water(world,px,py,pz)) {
        double bestDistance=std::numeric_limits<double>::max();
        bool foundDrySpawn=false;
        const double center=(double(size)-1.0)*0.5;
        for(int z=0;z<size;++z) for(int x=0;x<size;++x) {
            const int top=surface_y(world,x,z);
            if(top<63 || top+1>=Chunk::Height) continue;
            const BlockId floor=world.get_block(x,top-1,z);
            if(floor==BlockId::Air || floor==BlockId::Water || floor==BlockId::Lava) continue;
            if(world.get_block(x,top,z)!=BlockId::Air || world.get_block(x,top+1,z)!=BlockId::Air) continue;
            const double dx=double(x)-center,dz=double(z)-center;
            const double distance=dx*dx+dz*dz;
            if(distance<bestDistance) {
                bestDistance=distance; px=double(x)+0.5; pz=double(z)+0.5;
                py=double(top); foundDrySpawn=true;
            }
        }
        if(foundDrySpawn) motionX=motionY=motionZ=0.0;
    }
    // Never resume inside terrain, even if a save was made just before a
    // falling block landed or the world was edited around the player.
    if(!collision_boxes(world,player_box(px,py,pz)).empty()) {
        bool recovered=false;
        for(double lift=0.05;lift<=16.0001;lift+=0.05) {
            if(collision_boxes(world,player_box(px,py+lift,pz)).empty()) {
                py+=lift; recovered=true; break;
            }
        }
        if(!recovered) {
            px=double(size)*0.5; pz=double(size)*0.5;
            py=double(surface_y(world,int(px),int(pz)));
            motionX=motionY=motionZ=0.0;
        }
    }
    bool inventoryOpen=false;
    bool creativeOpen=false;
    bool paused=false;
    int pauseSelection=0;
    bool pauseSettings=false;
    int settingsSelection=0;
    float mouseSensitivity=0.0025f;
    float fieldOfView=70.0f;
    bool chatOpen=false;
    std::string chatInput;
    std::string chatMessage="Press - to open chat";
    std::string saveStatus="World loaded - custom .mcc format";
    bool thirdPerson=false;
    bool captured=true;
    bool inMenu=true;
    int menuSelection=0;
    bool optionsOpen=false;
    bool multiplayerOpen=false;
    bool requestQuit=false;
    float menuYaw=0.0f;
    while(!WindowShouldClose()) {
        const float dt=std::min(GetFrameTime(),0.10f);
        if(inMenu) {
            menuYaw+=dt*0.055f;
            Camera3D menuCamera{};
            const float cx=float(size)*BLOCK*0.5f, cz=float(size)*BLOCK*0.5f;
            menuCamera.position=Vector3{cx+std::sin(menuYaw)*48.0f,34.0f,cz+std::cos(menuYaw)*48.0f};
            menuCamera.target=Vector3{cx,14.0f,cz};
            menuCamera.up=Vector3{0,1,0}; menuCamera.fovy=65.0f; menuCamera.projection=CAMERA_PERSPECTIVE;
            BeginDrawing();
            ClearBackground(Color{110,160,205,255});
            BeginMode3D(menuCamera);
            const int center=int(size/2);
            for(int z=std::max(0,center-17);z<std::min(size,center+18);++z) {
                for(int x=std::max(0,center-17);x<std::min(size,center+18);++x) {
                    const int top=surface_y(world,x,z);
                    for(int y=std::max(0,top-5);y<top;++y) {
                        const BlockId id=world.get_block(x,y,z);
                        if(id!=BlockId::Air && (y==top-1 || world.get_block(x,y+1,z)==BlockId::Air))
                            draw_block(blockModels,world,id,x,y,z,block_center(x,y,z));
                    }
                }
            }
            EndMode3D();
            DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.34f));
            DrawText("MINECRAFT",GetScreenWidth()/2-220,92,64,RAYWHITE);
            DrawText("C++ EDITION",GetScreenWidth()/2-68,158,22,YELLOW);
            DrawText("A block world is waiting for you",GetScreenWidth()/2-150,194,18,RAYWHITE);
            const int bw=320,bh=44,gap=8,bx=GetScreenWidth()/2-bw/2,by=(optionsOpen||multiplayerOpen)?330:300;
            const char* labels[4]={"Singleplayer","Multiplayer","Options","Quit Game"};
            const int count=(optionsOpen||multiplayerOpen)?1:4;
            const bool menuPad=IsGamepadAvailable(0);
            if(menuPad) {
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP)) menuSelection=(menuSelection+count-1)%count;
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN)) menuSelection=(menuSelection+1)%count;
            }
            if(IsKeyPressed(KEY_UP)) menuSelection=(menuSelection+count-1)%count;
            if(IsKeyPressed(KEY_DOWN)) menuSelection=(menuSelection+1)%count;
            const bool menuConfirm=IsKeyPressed(KEY_ENTER)||(menuPad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
            const bool menuBack=IsKeyPressed(KEY_ESCAPE)||(menuPad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
            const Vector2 mouse=GetMousePosition();
            for(int i=0;i<count;++i) {
                const int y=by+i*(bh+gap);
                const Rectangle rect{float(bx),float(y),float(bw),float(bh)};
                const bool hover=CheckCollisionPointRec(mouse,rect);
                const bool focused=menuSelection==i;
                DrawRectangleRec(rect,(hover||focused)?Color{25,25,25,245}:Color{75,75,75,225});
                DrawRectangleLinesEx(rect,2,(hover||focused)?RAYWHITE:Color{25,25,25,255});
                const char* label=(optionsOpen||multiplayerOpen)?"Back":labels[i];
                DrawText(label,bx+(bw-MeasureText(label,22))/2,y+10,22,RAYWHITE);
                if((hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))||(focused&&menuConfirm)) {
                    if(optionsOpen||multiplayerOpen) { optionsOpen=false; multiplayerOpen=false; menuSelection=0; }
                    else if(i==0) { inMenu=false; captured=true; DisableCursor(); }
                    else if(i==1) { multiplayerOpen=true; menuSelection=0; }
                    else if(i==2) { optionsOpen=true; menuSelection=0; }
                    else if(i==3) requestQuit=true;
                }
            }
            if((optionsOpen||multiplayerOpen)&&menuBack) { optionsOpen=false; multiplayerOpen=false; menuSelection=0; }
            if(!optionsOpen&&!multiplayerOpen) DrawText("D-pad: select  |  A: confirm",GetScreenWidth()/2-105,by+4*(bh+gap)+8,15,RAYWHITE);
            else if(multiplayerOpen) {
                DrawText("Online and LAN multiplayer are not implemented yet",GetScreenWidth()/2-205,by+bh+22,17,RAYWHITE);
                DrawText("This screen will be connected when networking is added",GetScreenWidth()/2-225,by+bh+50,15,RAYWHITE);
            } else {
                DrawText("Graphics: 3D voxel panorama",GetScreenWidth()/2-125,by+bh+18,17,RAYWHITE);
                DrawText("World seed and display settings will be added here",GetScreenWidth()/2-215,by+bh+44,15,RAYWHITE);
            }
            DrawText("Mine C++ Clone  |  Inspired by the Java 1.12 era",12,GetScreenHeight()-28,14,RAYWHITE);
            EndDrawing();
            if(requestQuit) break;
            continue;
        }
        const bool gamepad=IsGamepadAvailable(0);
        if(IsKeyPressed(KEY_ESCAPE)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_MIDDLE_RIGHT))) {
            paused=!paused; pauseSettings=false;
            inventoryOpen=false; creativeOpen=false; chatOpen=false;
            captured=!paused;
            if(captured) DisableCursor(); else EnableCursor();
        }
        if(!paused && gamepad && IsGamepadButtonPressed(0,GAMEPAD_BUTTON_MIDDLE_LEFT)) {
#if defined(__SWITCH__)
            SwkbdConfig keyboard{};
            char keyboardText[128]{};
            if(R_SUCCEEDED(swkbdCreate(&keyboard,0))) {
                swkbdConfigMakePresetDefault(&keyboard);
                swkbdConfigSetGuideText(&keyboard,"Chat message or command");
                if(R_SUCCEEDED(swkbdShow(&keyboard,keyboardText,sizeof(keyboardText)))) {
                    chatInput=keyboardText;
                    std::istringstream commandStream(chatInput);
                    std::string command,arg;
                    commandStream>>command>>arg;
                    if(command=="/help") chatMessage="Commands: /help, /gamemode survival, /gamemode creative";
                    else if(command=="/gamemode" && (arg=="creative"||arg=="survival")) {
                        creativeMode=(arg=="creative");
                        chatMessage=creativeMode?"Game mode set to Creative":"Game mode set to Survival";
                    } else if(!chatInput.empty()) chatMessage="Unknown command: "+chatInput;
                }
                swkbdClose(&keyboard);
            }
#else
            chatOpen=!chatOpen; chatInput.clear();
            captured=!chatOpen;
            if(captured) DisableCursor(); else EnableCursor();
#endif
        }
        if(!paused && IsKeyPressed(KEY_ENTER) && !chatOpen) {
            chatOpen=true; chatInput.clear(); captured=false; EnableCursor();
        }
        if(chatOpen) {
            int key=GetCharPressed();
            while(key>0) {
                if(key>=32 && key<127 && chatInput.size()<120) chatInput.push_back(char(key));
                key=GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE) && !chatInput.empty()) chatInput.pop_back();
            if(IsKeyPressed(KEY_ESCAPE)) { chatOpen=false; captured=true; DisableCursor(); }
            if(IsKeyPressed(KEY_ENTER)) {
                std::istringstream commandStream(chatInput);
                std::string command,arg;
                commandStream>>command>>arg;
                if(command=="/help") chatMessage="Commands: /help, /gamemode survival, /gamemode creative";
                else if(command=="/gamemode" && (arg=="creative"||arg=="survival")) {
                    creativeMode=(arg=="creative");
                    chatMessage=creativeMode?"Game mode set to Creative":"Game mode set to Survival";
                }
                else if(!chatInput.empty()) chatMessage="Unknown command: "+chatInput;
                chatOpen=false; chatInput.clear(); captured=true; DisableCursor();
            }
        }
        if(paused) {
            constexpr int PAUSE_ACTIONS=4;
            if(!pauseSettings && (IsKeyPressed(KEY_UP)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP))))
                pauseSelection=(pauseSelection+PAUSE_ACTIONS-1)%PAUSE_ACTIONS;
            if(!pauseSettings && (IsKeyPressed(KEY_DOWN)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN))))
                pauseSelection=(pauseSelection+1)%PAUSE_ACTIONS;
            const bool confirm=!pauseSettings && (IsKeyPressed(KEY_ENTER)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN)));
            if(!pauseSettings && gamepad && IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                paused=false; captured=true; pauseSelection=0; DisableCursor();
            }
            const Vector2 pauseMouse=GetMousePosition();
            const int menuW=360,menuH=48,menuGap=12;
            const int menuX=GetScreenWidth()/2-menuW/2;
            const int menuY=GetScreenHeight()/2-70;
            for(int i=0;i<PAUSE_ACTIONS;++i) {
                const Rectangle item{float(menuX),float(menuY+i*(menuH+menuGap)),float(menuW),float(menuH)};
                const bool hover=CheckCollisionPointRec(pauseMouse,item);
                if(hover) pauseSelection=i;
                if(!pauseSettings && ((hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))||(pauseSelection==i&&confirm))) {
                    if(i==0) {
                        paused=false; captured=true; DisableCursor(); pauseSelection=0;
                    } else if(i==1) {
                        pauseSettings=true; settingsSelection=0;
                    } else if(i==2) {
                        if(save_world_file(world,px,py,pz,motionX,motionY,motionZ,yaw,pitch,inventory,selected))
                            saveStatus="Saved world and player: world.mcc";
                        else saveStatus="Save failed - storage unavailable";
                    } else {
                        if(save_world_file(world,px,py,pz,motionX,motionY,motionZ,yaw,pitch,inventory,selected)) {
                            saveStatus="Saved world and player: world.mcc";
                            paused=false; inMenu=true; menuSelection=0; captured=false; EnableCursor();
                        } else saveStatus="Save failed - return cancelled";
                    }
                    break;
                }
            }
            if(requestQuit) {
                // Leave the loop through the common cleanup path below.
            }
        }
        if(!paused && !chatOpen && (IsKeyPressed(KEY_E)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_LEFT)))) {
            inventoryOpen=!inventoryOpen; creativeOpen=false;
            captured=!inventoryOpen;
            if(captured) DisableCursor(); else EnableCursor();
        }
        if(!paused && !chatOpen && (IsKeyPressed(KEY_R)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_UP)))) {
            creativeOpen=!creativeOpen; inventoryOpen=false;
            captured=!creativeOpen;
            if(captured) DisableCursor(); else EnableCursor();
        }
        if(!paused && (IsKeyPressed(KEY_F5)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_THUMB)))) {
            thirdPerson=!thirdPerson;
        }
        if(captured) {
            const Vector2 md=GetMouseDelta();
            yaw+=md.x*mouseSensitivity;
            pitch=std::clamp(pitch-md.y*mouseSensitivity,-1.45f,1.45f);
            if(gamepad) {
                const float lookX=GetGamepadAxisMovement(0,GAMEPAD_AXIS_RIGHT_X);
                const float lookY=GetGamepadAxisMovement(0,GAMEPAD_AXIS_RIGHT_Y);
                if(std::abs(lookX)>0.16f) yaw+=lookX*2.2f*dt;
                if(std::abs(lookY)>0.16f) pitch=std::clamp(pitch-lookY*2.0f*dt,-1.45f,1.45f);
            }
        }
        float moveStrafe=0.0f,moveForward=0.0f;
        if(captured && IsKeyDown(KEY_A)) moveStrafe+=1.0f;
        if(captured && IsKeyDown(KEY_D)) moveStrafe-=1.0f;
        if(captured && IsKeyDown(KEY_W)) moveForward+=1.0f;
        if(captured && IsKeyDown(KEY_S)) moveForward-=1.0f;
        if(captured && gamepad) {
            const float stickX=GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_X);
            const float stickY=GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_Y);
            if(std::abs(stickX)>0.16f) moveStrafe-=stickX;
            if(std::abs(stickY)>0.16f) moveForward-=stickY;
        }
        const bool crouching=captured && IsKeyDown(KEY_LEFT_SHIFT);
        const bool sprintKey=captured && (IsKeyDown(KEY_LEFT_CONTROL) ||
            (gamepad&&IsGamepadButtonDown(0,GAMEPAD_BUTTON_LEFT_THUMB)));
        if(crouching) { moveStrafe*=0.3f; moveForward*=0.3f; }
        const bool jumping=captured && (IsKeyDown(KEY_SPACE) ||
            (gamepad&&IsGamepadButtonDown(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN)));
        if(!paused && !inventoryOpen && !creativeOpen && captured) physicsAccumulator+=dt;
        else { physicsAccumulator=0.0f; previousForward=false; }
        while(physicsAccumulator>=PLAYER_TICK) {
            if(sprintToggleTimer>0) --sprintToggleTimer;
            const bool forwardNow=moveForward>=0.8F;
            if(grounded && !crouching && !previousForward && forwardNow && !sprinting) {
                if(sprintToggleTimer<=0 && !sprintKey) sprintToggleTimer=7;
                else sprinting=true;
            }
            if(!sprinting && forwardNow && !crouching && sprintKey) sprinting=true;
            if(sprinting && (!forwardNow || collidedHorizontally || crouching)) sprinting=false;
            tick_vanilla_player(world,px,py,pz,motionX,motionY,motionZ,grounded,
                                collidedHorizontally,jumpTicks,moveStrafe,moveForward,
                                jumping,crouching,sprinting,yaw);
            previousForward=forwardNow;
            physicsAccumulator-=PLAYER_TICK;
            fallingBlockAccumulator+=PLAYER_TICK;
            if(fallingBlockAccumulator>=0.20f) {
                world.tick_falling_blocks();
                fallingBlockAccumulator-=0.20f;
            }
        }

        if(captured) {
            for(int i=0;i<HOTBAR_SIZE;++i) if(IsKeyPressed(KEY_ONE+i)) selected=i;
            const float wheel=GetMouseWheelMove();
            if(wheel>0.0f) selected=(selected+HOTBAR_SIZE-1)%HOTBAR_SIZE;
            else if(wheel<0.0f) selected=(selected+1)%HOTBAR_SIZE;
            if(gamepad) {
                // D-pad cycles the hotbar; L/R remain dedicated to block actions.
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_LEFT))
                    selected=(selected+HOTBAR_SIZE-1)%HOTBAR_SIZE;
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
                    selected=(selected+1)%HOTBAR_SIZE;
            }
        }
        if(inventoryOpen && gamepad) {
            static int inventoryCursor=0;
            if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_LEFT)) inventoryCursor=(inventoryCursor+INVENTORY_SIZE-1)%INVENTORY_SIZE;
            if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) inventoryCursor=(inventoryCursor+1)%INVENTORY_SIZE;
            if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP)) inventoryCursor=(inventoryCursor+INVENTORY_SIZE-9)%INVENTORY_SIZE;
            if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN)) inventoryCursor=(inventoryCursor+9)%INVENTORY_SIZE;
            if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) std::swap(inventory[selected],inventory[inventoryCursor]);
            if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                inventoryOpen=false; captured=true; DisableCursor();
            }
        }
        if(inventoryOpen&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const int panelW=520,panelH=310,panelX=GetScreenWidth()/2-panelW/2,panelY=GetScreenHeight()/2-panelH/2;
            const Vector2 mouse=GetMousePosition();
            for(int i=0;i<INVENTORY_SIZE;++i) {
                const int col=i%9,row=i/9,sx=panelX+18+col*54,sy=panelY+55+row*58;
                if(mouse.x>=sx&&mouse.x<sx+48&&mouse.y>=sy&&mouse.y<sy+50) {
                    if(i<HOTBAR_SIZE) selected=i;
                    else std::swap(inventory[selected],inventory[i]);
                    break;
                }
            }
        }

        Camera3D camera{};
        const float eyeHeight=crouching?1.54f:EYE_HEIGHT;
        const Vector3 lookDirection=Vector3{std::sin(yaw)*std::cos(pitch),std::sin(pitch),-std::cos(yaw)*std::cos(pitch)};
        if(thirdPerson) {
            camera.position=Vector3{float(px*BLOCK-lookDirection.x*8.0f),float((py+eyeHeight)*BLOCK+lookDirection.y*2.0f-lookDirection.y*8.0f),float(pz*BLOCK-lookDirection.z*8.0f)};
            camera.target=Vector3{float(px*BLOCK),float((py+eyeHeight)*BLOCK),float(pz*BLOCK)};
        } else {
            camera.position=Vector3{float(px*BLOCK),float((py+eyeHeight)*BLOCK),float(pz*BLOCK)};
            camera.target=Vector3Add(camera.position,Vector3Scale(lookDirection,1.0f));
        }
        camera.up=Vector3{0,1,0}; camera.fovy=fieldOfView; camera.projection=CAMERA_PERSPECTIVE;

        Vector3 hit{},previous{};
        const bool hasTarget=ray_block(GetScreenToWorldRay(Vector2{float(GetScreenWidth())*0.5f,float(GetScreenHeight())*0.5f},camera),world,hit,previous);
        if(!paused&&!inventoryOpen&&!creativeOpen&&captured&&hasTarget&&(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_TRIGGER_1)))) {
            const int bx=int(hit.x),by=int(hit.y),bz=int(hit.z);
            const BlockId broken=world.get_block(bx,by,bz);
            if(broken!=BlockId::Air&&broken!=BlockId::Bedrock&&broken!=BlockId::Water) {
                // Never delete a block if its drop cannot fit in the inventory.
                if(creativeMode || add_item(inventory,broken)) world.set_block(bx,by,bz,BlockId::Air);
            }
        }
        if(!paused&&!inventoryOpen&&captured&&hasTarget&&(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_TRIGGER_1)))) {
            const int bx=int(previous.x),by=int(previous.y),bz=int(previous.z);
            ItemStack& stack=inventory[selected];
            if(by>=1&&by<Chunk::Height&&stack.count>0&&stack.id!=BlockId::Air) {
                const PlayerAabb playerBounds=player_box(px,py,pz);
                const PlayerAabb targetBounds{double(bx),double(by),double(bz),double(bx+1),double(by+1),double(bz+1)};
                const bool overlaps=intersects(playerBounds,targetBounds);
                if(!overlaps&&world.set_block(bx,by,bz,stack.id)) {
                    if(is_wood_stairs(stack.id)) {
                        mc::world::BlockState state{};
                        state.facing=static_cast<std::uint8_t>(horizontal_facing_from_yaw(yaw));
                        // Looking at the underside of the support block places an upside-down stair.
                        state.shape=(previous.y<by)?1u:0u;
                        world.set_block_state(bx,by,bz,state);
                    }
                    if(!creativeMode) --stack.count;
                }
            }
        }

        // Dropped stacks have lightweight gravity, ground collision and pickup.
        if(!paused && !inventoryOpen && !creativeOpen && !chatOpen) {
            for(auto it=droppedItems.begin();it!=droppedItems.end();) {
                it->velocityY-=18.0f*dt;
                const float nextY=it->y+it->velocityY*dt;
                const int bx=int(std::floor(it->x)),bz=int(std::floor(it->z));
                const int below=int(std::floor(nextY-0.12f));
                if(below>=0 && below<Chunk::Height && block_info(world.get_block(bx,below,bz)).solid) {
                    it->y=float(below+1)+0.12f; it->velocityY=0.0f;
                } else it->y=nextY;
                const float dx=float(px)-it->x,dy=float(py+0.8)-it->y,dz=float(pz)-it->z;
                if(dx*dx+dy*dy+dz*dz<1.8f && add_item(inventory,it->id)) it=droppedItems.erase(it);
                else ++it;
            }
        }
        if(!paused&&!inventoryOpen&&!creativeOpen&&!chatOpen&&gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
            ItemStack& stack=inventory[selected];
            if(stack.count>0 && stack.id!=BlockId::Air) {
                const Vector3 direction{std::sin(yaw)*std::cos(pitch),std::sin(pitch),-std::cos(yaw)*std::cos(pitch)};
                droppedItems.push_back(DroppedItem{stack.id,float(px+direction.x*0.8f),float(py+1.0+direction.y*0.8f),float(pz+direction.z*0.8f),2.0f});
                if(!creativeMode) --stack.count;
                if(stack.count<=0) stack=ItemStack{};
            }
        }
        BeginDrawing();
        ClearBackground(Color{135,190,235,255});
        BeginMode3D(camera);
        const int minX=std::max(0,int(px)-14),maxX=std::min(size,int(px)+15);
        const int minZ=std::max(0,int(pz)-14),maxZ=std::min(size,int(pz)+15);
        const int minY=std::max(0,int(py)-18),maxY=std::min(Chunk::Height-1,int(py)+22);
        if(blockTexture.id!=0) {
            const int firstChunkX=minX/Chunk::Width,lastChunkX=maxX/Chunk::Width;
            const int firstChunkZ=minZ/Chunk::Depth,lastChunkZ=maxZ/Chunk::Depth;
            const int firstSection=minY/ChunkMeshBuilder::SectionHeight;
            const int lastSection=maxY/ChunkMeshBuilder::SectionHeight;
            for(int cz=firstChunkZ;cz<=lastChunkZ;++cz) for(int cx=firstChunkX;cx<=lastChunkX;++cx)
                for(int section=firstSection;section<=lastSection;++section) {
                    if(!section_in_frustum(camera,cx,cz,section,GetScreenWidth(),GetScreenHeight())) continue;
                    auto it=std::find_if(runtimeChunkMeshes.begin(),runtimeChunkMeshes.end(),
                        [&](const RuntimeChunkMesh& entry){return entry.x==cx&&entry.z==cz&&entry.section==section;});
                    if(it==runtimeChunkMeshes.end()) {
                        runtimeChunkMeshes.push_back(RuntimeChunkMesh{});
                        it=std::prev(runtimeChunkMeshes.end());
                        it->x=cx;it->z=cz;it->section=section;
                    }
                    if(!it->data.is_current(world,cx,cz)) {
                        if(it->model.meshCount>0) UnloadModel(it->model);
                        it->data=ChunkMeshBuilder::build(world,cx,cz,section);
                        it->model=make_chunk_model(it->data,blockTexture);
                    }
                    if(it->model.meshCount>0) DrawModel(it->model,Vector3{0,0,0},1.0f,WHITE);
                }
        }
        for(const auto& dropped:droppedItems)
            DrawCube(Vector3{dropped.x*BLOCK,(dropped.y+0.15f)*BLOCK,dropped.z*BLOCK},
                     0.28f*BLOCK,0.28f*BLOCK,0.28f*BLOCK,color_for(dropped.id));
        for(int z=minZ;z<maxZ;++z) for(int x=minX;x<maxX;++x)
            for(int y=minY;y<=maxY;++y) {
                const BlockId id=world.get_block(x,y,z);
                if(id==BlockId::Air) continue;
                if(blockTexture.id!=0 && is_chunk_mesh_block(id)) continue;
                if(world.get_block(x,y+1,z)!=BlockId::Air && world.get_block(x,y+1,z)!=BlockId::Water &&
                   world.get_block(x+1,y,z)!=BlockId::Air && world.get_block(x-1,y,z)!=BlockId::Air &&
                   world.get_block(x,y,z+1)!=BlockId::Air && world.get_block(x,y,z-1)!=BlockId::Air) continue;
                draw_block(blockModels,world,id,x,y,z,block_center(x,y,z));
            }
        if(blockTexture.id!=0) {
            const int firstChunkX=minX/Chunk::Width,lastChunkX=maxX/Chunk::Width;
            const int firstChunkZ=minZ/Chunk::Depth,lastChunkZ=maxZ/Chunk::Depth;
            const int firstSection=minY/ChunkMeshBuilder::SectionHeight;
            const int lastSection=maxY/ChunkMeshBuilder::SectionHeight;
            for(auto it=runtimeChunkMeshes.begin();it!=runtimeChunkMeshes.end();) {
                const bool visible=it->x>=firstChunkX&&it->x<=lastChunkX&&
                    it->z>=firstChunkZ&&it->z<=lastChunkZ&&
                    it->section>=firstSection&&it->section<=lastSection;
                if(!visible) {
                    if(it->model.meshCount>0)UnloadModel(it->model);
                    it=runtimeChunkMeshes.erase(it);
                } else ++it;
            }
        }
        EndMode3D();
        DrawRectangle(0,0,GetScreenWidth(),38,Fade(BLACK,0.35f));
        DrawText("MINE C++ CLONE  |  SURVIVAL TEST",14,10,18,RAYWHITE);
        DrawText("World autosaves to world.mcc when you exit",14,48,14,RAYWHITE);
        DrawText("WASD / Stick: move | A: jump | B: drop | X: inventory | Y: creative | L: place | R: break | +: pause | -: chat",14,GetScreenHeight()-28,13,RAYWHITE);
        DrawRectangle(GetScreenWidth()/2-1,GetScreenHeight()/2-8,2,16,RAYWHITE);
        DrawRectangle(GetScreenWidth()/2-8,GetScreenHeight()/2-1,16,2,RAYWHITE);
        const int barW=52,barX=GetScreenWidth()/2-(barW*HOTBAR_SIZE)/2,barY=GetScreenHeight()-100;
        for(int i=0;i<HOTBAR_SIZE;++i) {
            const ItemStack& stack=inventory[i];
            DrawRectangle(barX+i*barW,barY,48,56,Fade(BLACK,0.72f));
            DrawRectangleLines(barX+i*barW,barY,48,56,i==selected?YELLOW:GRAY);
            if(stack.count>0) DrawRectangle(barX+i*barW+10,barY+13,28,28,color_for(stack.id));
            DrawText(TextFormat("%i",i+1),barX+i*barW+3,barY+3,11,RAYWHITE);
            if(stack.count>0) DrawText(TextFormat("%i",stack.count),barX+i*barW+29,barY+41,12,RAYWHITE);
        }
        if(creativeOpen) {
            constexpr int PAGE_SIZE=48, GRID_COLS=8;
            static int creativeSelection=0;
            if(gamepad) {
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_LEFT)) creativeSelection=(creativeSelection+PAGE_SIZE-1)%PAGE_SIZE;
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) creativeSelection=(creativeSelection+1)%PAGE_SIZE;
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP)) creativeSelection=(creativeSelection+PAGE_SIZE-GRID_COLS)%PAGE_SIZE;
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN)) creativeSelection=(creativeSelection+GRID_COLS)%PAGE_SIZE;
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) { creativeOpen=false; captured=true; DisableCursor(); }
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                    const int raw=1+creativePage*PAGE_SIZE+creativeSelection;
                    if(raw<int(BLOCK_ID_COUNT)) inventory[selected]={static_cast<BlockId>(raw),64};
                    creativeOpen=false; captured=true; DisableCursor();
                }
            }
            if(gamepad && IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_TRIGGER_1))
                creativePage=(creativePage+1)%std::max(1,(int(BLOCK_ID_COUNT)-1+PAGE_SIZE-1)/PAGE_SIZE);
            if(gamepad && IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_TRIGGER_1))
                creativePage=(creativePage+std::max(1,(int(BLOCK_ID_COUNT)-1+PAGE_SIZE-1)/PAGE_SIZE)-1)%std::max(1,(int(BLOCK_ID_COUNT)-1+PAGE_SIZE-1)/PAGE_SIZE);
            const int totalPages=(int(BLOCK_ID_COUNT)-1+PAGE_SIZE-1)/PAGE_SIZE;
            const int panelW=704,panelH=470,panelX=GetScreenWidth()/2-panelW/2,panelY=GetScreenHeight()/2-panelH/2;
            creativePage=std::clamp(creativePage,0,totalPages-1);
            if(IsKeyPressed(KEY_PAGE_DOWN)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_TRIGGER_1)))
                creativePage=(creativePage+1)%totalPages;
            if(IsKeyPressed(KEY_PAGE_UP)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_TRIGGER_1)))
                creativePage=(creativePage+totalPages-1)%totalPages;
            DrawRectangle(panelX,panelY,panelW,panelH,Fade(Color{38,38,42,255},0.98f));
            DrawRectangleLines(panelX,panelY,panelW,panelH,RAYWHITE);
            DrawText("CREATIVE INVENTORY",panelX+20,panelY+14,25,RAYWHITE);
            DrawText(TextFormat("Page %i / %i",creativePage+1,totalPages),panelX+panelW-125,panelY+20,16,YELLOW);
            const Vector2 mouse=GetMousePosition();
            for(int i=0;i<PAGE_SIZE;++i) {
                const int raw=1+creativePage*PAGE_SIZE+i;
                if(raw>=int(BLOCK_ID_COUNT)) break;
                const int col=i%GRID_COLS,row=i/GRID_COLS;
                const int sx=panelX+18+col*84,sy=panelY+54+row*56;
                const Rectangle cell{float(sx),float(sy),78.0f,50.0f};
                const bool hover=CheckCollisionPointRec(mouse,cell);
                const bool focused=gamepad && (i==creativeSelection);
                DrawRectangleRec(cell,(hover||focused)?Color{90,90,96,255}:Color{58,58,64,255});
                DrawRectangleLinesEx(cell,1,(hover||focused)?YELLOW:GRAY);
                const BlockId id=static_cast<BlockId>(raw);
                DrawRectangle(sx+5,sy+7,24,24,color_for(id));
                const std::string label(block_info(id).name);
                DrawText(label.c_str(),sx+32,sy+8,9,RAYWHITE);
                if(hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    inventory[selected]={id,64};
                    creativeOpen=false;
                    captured=true;
                    DisableCursor();
                }
            }
            DrawText("Click a block to put 64 in the selected hotbar slot",panelX+20,panelY+402,14,LIGHTGRAY);
            DrawText("Page Up / Page Down or L1 / R1: pages    Y: close",panelX+20,panelY+430,14,RAYWHITE);
        }
        if(paused) {
            DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.72f));
            DrawText("GAME MENU",GetScreenWidth()/2-110,GetScreenHeight()/2-145,34,RAYWHITE);
            const char* pauseLabels[4]={"Continue","Settings","Save World","Save and Return to Title"};
            const int menuW=360,menuH=48,menuGap=12;
            const int menuX=GetScreenWidth()/2-menuW/2;
            const int menuY=GetScreenHeight()/2-70;
            const Vector2 pauseMouse=GetMousePosition();
            for(int i=0;i<4;++i) {
                const int y=menuY+i*(menuH+menuGap);
                const Rectangle item{float(menuX),float(y),float(menuW),float(menuH)};
                const bool hover=CheckCollisionPointRec(pauseMouse,item);
                const bool selectedItem=pauseSelection==i||hover;
                DrawRectangleRec(item,selectedItem?Color{45,45,45,250}:Color{85,85,85,235});
                DrawRectangleLinesEx(item,2,selectedItem?YELLOW:LIGHTGRAY);
                DrawText(pauseLabels[i],menuX+(menuW-MeasureText(pauseLabels[i],22))/2,y+11,22,RAYWHITE);
            }
            DrawText(saveStatus.c_str(),GetScreenWidth()/2-MeasureText(saveStatus.c_str(),16)/2,
                     menuY+3*(menuH+menuGap)+4,16,YELLOW);
            DrawText("D-pad: select   A / Enter: confirm   B / Esc: resume",
                     GetScreenWidth()/2-230,menuY+4*(menuH+menuGap)+18,14,RAYWHITE);
            DrawText("World format: Mine C++ Clone (.mcc)",GetScreenWidth()/2-150,
                     menuY+4*(menuH+menuGap)+40,14,LIGHTGRAY);
            if(pauseSettings) {
                DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.82f));
                DrawText("SETTINGS",GetScreenWidth()/2-85,GetScreenHeight()/2-105,30,RAYWHITE);
                const char* settingNames[2]={"Field of view","Mouse sensitivity"};
                const float values[2]={fieldOfView,mouseSensitivity*1000.0f};
                for(int i=0;i<2;++i) {
                    const int y=GetScreenHeight()/2-35+i*58;
                    DrawText(settingNames[i],GetScreenWidth()/2-190,y,20,RAYWHITE);
                    DrawText(TextFormat("%.1f",values[i]),GetScreenWidth()/2+100,y,20,YELLOW);
                }
                DrawText("Left / Right: change    B / Esc: back",GetScreenWidth()/2-180,
                         GetScreenHeight()/2+105,15,LIGHTGRAY);
                if(IsKeyPressed(KEY_LEFT)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_LEFT))) {
                    if(settingsSelection==0) fieldOfView=std::max(50.0f,fieldOfView-5.0f);
                    else mouseSensitivity=std::max(0.0008f,mouseSensitivity-0.0002f);
                }
                if(IsKeyPressed(KEY_RIGHT)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_RIGHT))) {
                    if(settingsSelection==0) fieldOfView=std::min(110.0f,fieldOfView+5.0f);
                    else mouseSensitivity=std::min(0.0060f,mouseSensitivity+0.0002f);
                }
                if(IsKeyPressed(KEY_UP)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP))) settingsSelection=0;
                if(IsKeyPressed(KEY_DOWN)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN))) settingsSelection=1;
                if(IsKeyPressed(KEY_ESCAPE)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) pauseSettings=false;
            }
        }
        if(chatOpen) {
            DrawRectangle(12,GetScreenHeight()-82,GetScreenWidth()-24,66,Fade(BLACK,0.78f));
            DrawText(("> "+chatInput+"_").c_str(),24,GetScreenHeight()-70,20,RAYWHITE);
            DrawText("Enter: send    Esc: close",24,GetScreenHeight()-42,13,LIGHTGRAY);
        } else if(!chatMessage.empty()) {
            DrawText(chatMessage.c_str(),14,GetScreenHeight()-50,14,YELLOW);
        }
        if(inventoryOpen) {
            const int panelW=520,panelH=310,panelX=GetScreenWidth()/2-panelW/2,panelY=GetScreenHeight()/2-panelH/2;
            DrawRectangle(panelX,panelY,panelW,panelH,Fade(DARKGRAY,0.96f));
            DrawRectangleLines(panelX,panelY,panelW,panelH,RAYWHITE);
            DrawText("INVENTORY - X to close",panelX+18,panelY+14,22,RAYWHITE);
            for(int i=0;i<INVENTORY_SIZE;++i) {
                const int col=i%9,row=i/9,sx=panelX+18+col*54,sy=panelY+55+row*58;
                DrawRectangle(sx,sy,48,50,Fade(BLACK,0.7f));
                DrawRectangleLines(sx,sy,48,50,i==selected?YELLOW:GRAY);
                if(inventory[i].count>0) {
                    DrawRectangle(sx+10,sy+8,28,28,color_for(inventory[i].id));
                    DrawText(TextFormat("%i",inventory[i].count),sx+27,sy+34,12,RAYWHITE);
                }
            }
            DrawText("CRAFTING: recipe area is built into this inventory (recipes coming next)",panelX+18,panelY+252,14,YELLOW);
            DrawText("Hotbar: keys 1-9 | Y: creative menu",panelX+18,panelY+286,14,RAYWHITE);
        }
        DrawText(TextFormat("XYZ %.1f / %.1f / %.1f",px,py,pz),14,48,16,RAYWHITE);
        if(!captured) DrawText("Cursor released - press ESC to resume",GetScreenWidth()/2-170,GetScreenHeight()/2+24,18,YELLOW);
        DrawFPS(GetScreenWidth()-90,12);
        EndDrawing();
        if(requestQuit) break;
    }
    if(!requestQuit) save_world_file(world,px,py,pz,motionX,motionY,motionZ,yaw,pitch,inventory,selected);
    for(auto& cached:runtimeChunkMeshes) if(cached.model.meshCount>0) UnloadModel(cached.model);
    for(unsigned int i=1;i<BLOCK_ID_COUNT;++i) {
        if(blockModels[i].meshCount>0) {
            for(int material=0;material<blockModels[i].materialCount;++material)
                blockModels[i].materials[material].maps[MATERIAL_MAP_DIFFUSE].texture.id=0;
            UnloadModel(blockModels[i]);
        }
    }
    if(blockTexture.id!=0) UnloadTexture(blockTexture);
#if defined(__SWITCH__)
    romfsExit();
#endif
    EnableCursor();
    CloseWindow();
    return 0;
}
}
