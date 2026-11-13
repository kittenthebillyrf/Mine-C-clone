#include "TerrainViewer.hpp"
#include "BlockAtlas.hpp"
#include "raylib.h"
#include "raymath.h"
#include "../world/World.hpp"
#include "../world/generation/ChunkGenerator.hpp"

#include <algorithm>
#include <limits>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <deque>
#include <utility>
#include <vector>

namespace mc::render {
namespace {
constexpr float BLOCK=2.0f;
constexpr float PLAYER_HEIGHT=3.6f;
constexpr float EYE_HEIGHT=3.15f;
constexpr float REACH=16.0f; // 8 blocks at the current 2-unit block scale.
constexpr float MAX_PITCH=1.52f;
constexpr int HOTBAR_SIZE=9;
constexpr int INVENTORY_SIZE=36;
constexpr std::size_t MAX_RESIDENT_CHUNKS=100;
constexpr int KEEP_CHUNK_RADIUS=3;
struct BreakParticle { Vector3 position{}; Vector3 velocity{}; float life=0.0f; float maxLife=0.45f; Color color=WHITE; };
struct ItemStack { mc::world::BlockId id=mc::world::BlockId::Air; int count=0; };
Color color_for(mc::world::BlockId id) {
    using mc::world::BlockId;
    switch(id) {
        case BlockId::Grass:return Color{82,160,65,255};
        case BlockId::Dirt:return Color{128,87,55,255};
        case BlockId::Stone:return Color{125,127,132,255};
        case BlockId::Sand:return Color{210,194,139,255};
        case BlockId::Water:return Color{55,115,205,170};
        case BlockId::Bedrock:return Color{55,55,58,255};
        case BlockId::Wood:return Color{126,91,54,255};
        case BlockId::Leaves:return Color{55,135,58,220};
        case BlockId::Gravel:return Color{115,113,110,255};
        case BlockId::CoalOre:return Color{65,65,68,255};
        case BlockId::IronOre:return Color{190,135,105,255};
        case BlockId::Planks:return Color{174,132,78,255};
        case BlockId::Stick:return Color{185,145,91,255};
        case BlockId::CraftingTable:return Color{145,99,55,255};
        default:return BLANK;
    }
}
Vector3 block_center(int x,int y,int z) { return Vector3{(float(x)+0.5f)*BLOCK,(float(y)+0.5f)*BLOCK,(float(z)+0.5f)*BLOCK}; }
int surface_y(const mc::world::World& world,int x,int z) {
    for(int y=mc::world::Chunk::Height-1;y>=0;--y) {
        const auto id=world.get_block(x,y,z);
        if(id!=mc::world::BlockId::Air&&id!=mc::world::BlockId::Water) return y+1;
    }
    return 1;
}
bool is_solid(const mc::world::World& world,int x,int y,int z) {
    using mc::world::BlockId;
    const BlockId id=world.get_block(x,y,z);
    return id!=BlockId::Air && id!=BlockId::Water;
}

// Player coordinates use block units horizontally and world units vertically.
bool player_collides(const mc::world::World& world,float px,float feetY,float pz) {
    constexpr float radius=0.30f*BLOCK;
    constexpr float height=1.80f*BLOCK;
    constexpr float epsilon=0.025f;
    const int minX=int(std::floor((px*BLOCK-radius)/BLOCK));
    const int maxX=int(std::floor((px*BLOCK+radius)/BLOCK));
    const int minY=int(std::floor((feetY+epsilon)/BLOCK));
    const int maxY=int(std::floor((feetY+height-epsilon)/BLOCK));
    const int minZ=int(std::floor((pz*BLOCK-radius)/BLOCK));
    const int maxZ=int(std::floor((pz*BLOCK+radius)/BLOCK));
    for(int y=minY;y<=maxY;++y)
        for(int z=minZ;z<=maxZ;++z)
            for(int x=minX;x<=maxX;++x)
                if(is_solid(world,x,y,z)) return true;
    return false;
}

bool player_in_water(const mc::world::World& world,float px,float feetY,float pz) {
    constexpr float radius=0.28f*BLOCK;
    constexpr float height=1.75f*BLOCK;
    const int minX=int(std::floor((px*BLOCK-radius)/BLOCK));
    const int maxX=int(std::floor((px*BLOCK+radius)/BLOCK));
    const int minY=int(std::floor((feetY+0.05f)/BLOCK));
    const int maxY=int(std::floor((feetY+height)/BLOCK));
    const int minZ=int(std::floor((pz*BLOCK-radius)/BLOCK));
    const int maxZ=int(std::floor((pz*BLOCK+radius)/BLOCK));
    for(int y=minY;y<=maxY;++y)
        for(int z=minZ;z<=maxZ;++z)
            for(int x=minX;x<=maxX;++x)
                if(world.get_block(x,y,z)==mc::world::BlockId::Water) return true;
    return false;
}


struct CraftResult { mc::world::BlockId id=mc::world::BlockId::Air; int count=0; };
CraftResult match_recipe(const ItemStack* grid) {
    using mc::world::BlockId;
    int total=0,wood=0,planks=0;
    for(int i=0;i<4;++i) {
        total+=grid[i].count;
        if(grid[i].count>0 && grid[i].id==BlockId::Wood) wood+=grid[i].count;
        if(grid[i].count>0 && grid[i].id==BlockId::Planks) planks+=grid[i].count;
    }
    if(total==1 && wood==1) return {BlockId::Planks,4};
    if(total==2 && planks==2 && grid[0].id==BlockId::Planks && grid[2].id==BlockId::Planks)
        return {BlockId::Stick,4};
    if(total==4 && planks==4) return {BlockId::CraftingTable,1};
    return {};
}
bool add_stack(ItemStack* inventory,mc::world::BlockId id,int count) {
    int capacity=0;
    for(int i=0;i<INVENTORY_SIZE;++i) {
        if(inventory[i].count==0) capacity+=64;
        else if(inventory[i].id==id) capacity+=64-inventory[i].count;
    }
    if(capacity<count) return false;
    int remaining=count;
    for(int i=0;i<INVENTORY_SIZE&&remaining>0;++i) {
        if(inventory[i].id==id&&inventory[i].count>0&&inventory[i].count<64) {
            const int moved=std::min(remaining,64-inventory[i].count);
            inventory[i].count+=moved; remaining-=moved;
        }
    }
    for(int i=0;i<INVENTORY_SIZE&&remaining>0;++i) {
        if(inventory[i].count==0) {
            const int moved=std::min(remaining,64);
            inventory[i]={id,moved}; remaining-=moved;
        }
    }
    return remaining==0;
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
    // Amanatides-Woo voxel traversal: visit every crossed block exactly once.
    // Unlike fixed 0.08-unit stepping, this cannot skip thin edge intersections.
    const Vector3 direction=Vector3Normalize(ray.direction);
    if(Vector3LengthSqr(direction)<0.000001f) return false;

    int x=int(std::floor(ray.position.x/BLOCK));
    int y=int(std::floor(ray.position.y/BLOCK));
    int z=int(std::floor(ray.position.z/BLOCK));
    const int stepX=(direction.x>0.0f)?1:((direction.x<0.0f)?-1:0);
    const int stepY=(direction.y>0.0f)?1:((direction.y<0.0f)?-1:0);
    const int stepZ=(direction.z>0.0f)?1:((direction.z<0.0f)?-1:0);
    const float infinity=std::numeric_limits<float>::infinity();
    const float deltaX=(stepX==0)?infinity:BLOCK/std::abs(direction.x);
    const float deltaY=(stepY==0)?infinity:BLOCK/std::abs(direction.y);
    const float deltaZ=(stepZ==0)?infinity:BLOCK/std::abs(direction.z);
    float maxX=(stepX>0?float(x+1)*BLOCK:float(x)*BLOCK);
    float maxY=(stepY>0?float(y+1)*BLOCK:float(y)*BLOCK);
    float maxZ=(stepZ>0?float(z+1)*BLOCK:float(z)*BLOCK);
    float nextX=(stepX==0)?infinity:(maxX-ray.position.x)/direction.x;
    float nextY=(stepY==0)?infinity:(maxY-ray.position.y)/direction.y;
    float nextZ=(stepZ==0)?infinity:(maxZ-ray.position.z)/direction.z;

    int previousX=x,previousY=y,previousZ=z;
    float travelled=0.0f;
    while(travelled<=REACH) {
        const auto id=world.get_block(x,y,z);
        if(id!=mc::world::BlockId::Air && id!=mc::world::BlockId::Water) {
            hit=Vector3{float(x),float(y),float(z)};
            previous=Vector3{float(previousX),float(previousY),float(previousZ)};
            return true;
        }

        previousX=x; previousY=y; previousZ=z;
        if(nextX<=nextY && nextX<=nextZ) {
            travelled=nextX; nextX+=deltaX; x+=stepX;
        } else if(nextY<=nextZ) {
            travelled=nextY; nextY+=deltaY; y+=stepY;
        } else {
            travelled=nextZ; nextZ+=deltaZ; z+=stepZ;
        }
    }
    return false;
}
}
int run_terrain_viewer(int map_size,std::uint32_t seed) {
    using namespace mc::world;
    const int size=std::clamp(map_size,256,4096);
    World world;
    const generation::ChunkGenerator generator(seed);
    std::deque<ChunkPos> pendingChunks;
    std::deque<ChunkPos> residentOrder;
    std::vector<BreakParticle> breakParticles;
    { std::ifstream saveFile("world.mcc",std::ios::binary); if(saveFile) world.load(saveFile); }
    const int spawnChunkX=(size/2)/Chunk::Width, spawnChunkZ=(size/2)/Chunk::Depth;
    if(world.find_chunk(spawnChunkX,spawnChunkZ)==nullptr) {
        if(!world.load_chunk_file(spawnChunkX,spawnChunkZ,"world_chunks"))
            generator.generate_chunk(world,spawnChunkX,spawnChunkZ);
    }
    for(const ChunkPos& pos:world.chunk_positions()) residentOrder.push_back(pos);
    const auto ensure_chunks = [&](int world_x,int world_z) {
        const int centerX=world_x/Chunk::Width, centerZ=world_z/Chunk::Depth;
        for(int cz=std::max(0,centerZ-2);cz<=std::min((size-1)/Chunk::Depth,centerZ+2);++cz)
            for(int cx=std::max(0,centerX-2);cx<=std::min((size-1)/Chunk::Width,centerX+2);++cx) {
                if(world.find_chunk(cx,cz)!=nullptr) continue;
                const bool queued=std::any_of(pendingChunks.begin(),pendingChunks.end(),
                    [&](const ChunkPos& p){ return p.x==cx&&p.z==cz; });
                if(!queued) pendingChunks.push_back(ChunkPos{cx,cz});
            }
    };
    ensure_chunks(size/2,size/2);

    InitWindow(1280,720,"Mine C++ Clone");
    BlockAtlasAssets& blockAtlas = load_block_atlas();
#ifdef __SWITCH__
    SetTargetFPS(60);
    constexpr int renderRadius=7;
    constexpr int verticalRadius=12;
    constexpr int menuRadius=12;
#else
    SetTargetFPS(120);
    constexpr int renderRadius=14;
    constexpr int verticalRadius=20;
    constexpr int menuRadius=17;
#endif
    DisableCursor();
    float px=float(size)*0.5f+0.5f,pz=float(size)*0.5f+0.5f;
    float py=float(surface_y(world,int(px),int(pz)))*BLOCK;
    float vx=0.0f,vz=0.0f,vy=0.0f,yaw=0.0f,pitch=-0.18f;
    bool grounded=player_collides(world,px,py-0.05f,pz);
    int selected=0;
    int miningX=-1,miningY=-1,miningZ=-1;
    float miningProgress=0.0f;
    ItemStack inventory[INVENTORY_SIZE]{};
    ItemStack craftingGrid[4]{};
    const BlockId startingItems[5]={BlockId::Grass,BlockId::Dirt,BlockId::Stone,BlockId::Sand,BlockId::Water};
    for(int i=0;i<5;++i) inventory[i]={startingItems[i],32};
    bool inventoryOpen=false;
    bool creativeOpen=false;
    bool paused=false;
    bool pauseOptionsOpen=false;
    int pauseSelection=0;
    float cameraFov=70.0f;
    bool thirdPerson=false;
    bool captured=true;
    bool inMenu=true;
    bool optionsOpen=false;
    bool multiplayerOpen=false;
    bool requestQuit=false;
    float menuYaw=0.0f;
    int menuSelection=0;
    while(!WindowShouldClose()) {
        const float dt=std::min(GetFrameTime(),0.04f);
        const bool gamepad=IsGamepadAvailable(0);
        ensure_chunks(int(px),int(pz));
        if(!pendingChunks.empty()) {
            const ChunkPos next=pendingChunks.front();
            pendingChunks.pop_front();
            if(world.find_chunk(next.x,next.z)==nullptr) {
                if(!world.load_chunk_file(next.x,next.z,"world_chunks"))
                    generator.generate_chunk(world,next.x,next.z);
                if(world.find_chunk(next.x,next.z)!=nullptr) residentOrder.push_back(next);
            }
        }
        // Bound RAM while retaining a safety ring around the active player.
        std::size_t evictionAttempts=residentOrder.size();
        while(world.chunk_count()>MAX_RESIDENT_CHUNKS && !residentOrder.empty() && evictionAttempts-->0) {
            const ChunkPos candidate=residentOrder.front();
            residentOrder.pop_front();
            const int centerX=int(px)/Chunk::Width,centerZ=int(pz)/Chunk::Depth;
            if(std::abs(candidate.x-centerX)<=KEEP_CHUNK_RADIUS &&
               std::abs(candidate.z-centerZ)<=KEEP_CHUNK_RADIUS) {
                residentOrder.push_back(candidate);
                if(residentOrder.size()<=MAX_RESIDENT_CHUNKS) break;
                continue;
            }
            if(world.find_chunk(candidate.x,candidate.z)) {
                if(world.save_chunk_file(candidate.x,candidate.z,"world_chunks"))
                    world.unload_chunk(candidate.x,candidate.z);
                else residentOrder.push_back(candidate);
            }
            if(residentOrder.empty()) break;
        }
        for(auto it=breakParticles.begin();it!=breakParticles.end();) {
            it->life+=dt;
            it->velocity.y-=18.0f*dt;
            it->position=Vector3Add(it->position,Vector3Scale(it->velocity,dt));
            if(it->life>=it->maxLife) it=breakParticles.erase(it); else ++it;
        }
        if(inMenu) {
            menuYaw+=dt*0.055f;
            if(IsKeyPressed(KEY_UP)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP)))
                menuSelection=(menuSelection+3)%4;
            if(IsKeyPressed(KEY_DOWN)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN)))
                menuSelection=(menuSelection+1)%4;
            const bool menuActivate=IsKeyPressed(KEY_ENTER)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
            Camera3D menuCamera{};
            const float cx=float(size)*BLOCK*0.5f, cz=float(size)*BLOCK*0.5f;
            menuCamera.position=Vector3{cx+std::sin(menuYaw)*48.0f,34.0f,cz+std::cos(menuYaw)*48.0f};
            menuCamera.target=Vector3{cx,14.0f,cz};
            menuCamera.up=Vector3{0,1,0}; menuCamera.fovy=65.0f; menuCamera.projection=CAMERA_PERSPECTIVE;
            BeginDrawing();
            ClearBackground(Color{110,160,205,255});
            BeginMode3D(menuCamera);
            const int center=int(size/2);
            for(int z=std::max(0,center-menuRadius);z<std::min(size,center+menuRadius+1);++z) {
                for(int x=std::max(0,center-menuRadius);x<std::min(size,center+menuRadius+1);++x) {
                    const int top=surface_y(world,x,z);
                    for(int y=std::max(0,top-5);y<top;++y) {
                        const BlockId id=world.get_block(x,y,z);
                        if(id!=BlockId::Air && (y==top-1 || world.get_block(x,y+1,z)==BlockId::Air)) {
                            const Vector3 center = block_center(x,y,z);
                            if (!draw_textured_block(blockAtlas,id,center)) DrawCube(center,BLOCK,BLOCK,BLOCK,color_for(id));
                        }
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
            const Vector2 mouse=GetMousePosition();
            for(int i=0;i<count;++i) {
                const int y=by+i*(bh+gap);
                const Rectangle rect{float(bx),float(y),float(bw),float(bh)};
                const bool hover=CheckCollisionPointRec(mouse,rect);
                if(hover) menuSelection=i;
                const bool active=(optionsOpen||multiplayerOpen)||menuSelection==i;
                DrawRectangleRec(rect,active?Color{25,25,25,245}:Color{75,75,75,225});
                DrawRectangleLinesEx(rect,2,active?RAYWHITE:Color{25,25,25,255});
                const char* label=(optionsOpen||multiplayerOpen)?"Back":labels[i];
                DrawText(label,bx+(bw-MeasureText(label,22))/2,y+10,22,RAYWHITE);
                if((hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))||(menuActivate&&active)) {
                    if(optionsOpen||multiplayerOpen) { optionsOpen=false; multiplayerOpen=false; menuSelection=0; }
                    else if(i==0) { inMenu=false; captured=true; DisableCursor(); }
                    else if(i==1) { multiplayerOpen=true; menuSelection=0; }
                    else if(i==2) { optionsOpen=true; menuSelection=0; }
                    else if(i==3) requestQuit=true;
                }
            }
            if(!optionsOpen&&!multiplayerOpen) DrawText("Multiplayer is coming in a future update",GetScreenWidth()/2-178,by+4*(bh+gap)+8,15,RAYWHITE);
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
        const bool pausePressed=IsKeyPressed(KEY_ESCAPE)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_MIDDLE_RIGHT));
        if(pausePressed) {
            if(!paused) { paused=true; pauseOptionsOpen=false; pauseSelection=0; inventoryOpen=false; creativeOpen=false; captured=false; EnableCursor(); }
            else if(pauseOptionsOpen) { pauseOptionsOpen=false; pauseSelection=0; }
            else { paused=false; captured=true; DisableCursor(); }
        }
        if(paused) {
            const bool up=IsKeyPressed(KEY_UP)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_UP));
            const bool down=IsKeyPressed(KEY_DOWN)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_FACE_DOWN));
            const bool accept=IsKeyPressed(KEY_ENTER)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
            const bool back=IsKeyPressed(KEY_BACKSPACE)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
            if(pauseOptionsOpen) {
                if(back) { pauseOptionsOpen=false; pauseSelection=0; }
                if(up) cameraFov=std::min(100.0f,cameraFov+2.0f);
                if(down) cameraFov=std::max(50.0f,cameraFov-2.0f);
            } else {
                if(up) pauseSelection=(pauseSelection+2)%3;
                if(down) pauseSelection=(pauseSelection+1)%3;
                const int bw=300,bh=48,gap=12,bx=GetScreenWidth()/2-bw/2,by=GetScreenHeight()/2-5;
                const Vector2 mouse=GetMousePosition();
                for(int i=0;i<3;++i) {
                    const Rectangle r{float(bx),float(by+i*(bh+gap)),float(bw),float(bh)};
                    if(CheckCollisionPointRec(mouse,r)) {
                        pauseSelection=i;
                        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                            if(i==0) { paused=false; captured=true; DisableCursor(); }
                            else if(i==1) { pauseOptionsOpen=true; pauseSelection=0; }
                            else {
                                for(const ChunkPos& pos:world.chunk_positions()) world.save_chunk_file(pos.x,pos.z,"world_chunks");
                                paused=false; pauseOptionsOpen=false; inMenu=true; captured=false; EnableCursor(); menuSelection=0;
                            }
                        }
                    }
                }
                if(accept) {
                    if(pauseSelection==0) { paused=false; captured=true; DisableCursor(); }
                    else if(pauseSelection==1) { pauseOptionsOpen=true; pauseSelection=0; }
                    else {
                        for(const ChunkPos& pos:world.chunk_positions()) world.save_chunk_file(pos.x,pos.z,"world_chunks");
                        paused=false; pauseOptionsOpen=false; inMenu=true; captured=false; EnableCursor(); menuSelection=0;
                    }
                }
            }
        }
        if(!paused && (IsKeyPressed(KEY_E)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_UP)))) {
            inventoryOpen=!inventoryOpen; creativeOpen=false;
            captured=!inventoryOpen;
            if(captured) DisableCursor(); else EnableCursor();
        }
        if(!paused && IsKeyPressed(KEY_R)) {
            creativeOpen=!creativeOpen; inventoryOpen=false;
            captured=!creativeOpen;
            if(captured) DisableCursor(); else EnableCursor();
        }
        if(!paused && (IsKeyPressed(KEY_F5)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_THUMB)))) {
            thirdPerson=!thirdPerson;
        }
        const float controlDt=(paused||inventoryOpen||creativeOpen)?0.0f:dt;
        const bool crouching=IsKeyDown(KEY_LEFT_SHIFT);
        const bool swimming=player_in_water(world,px,py,pz);

        if(captured) {
            const Vector2 md=GetMouseDelta();
            yaw+=md.x*0.0025f;
            pitch=std::clamp(pitch-md.y*0.0025f,-MAX_PITCH,MAX_PITCH);
            if(gamepad) {
                const float lookX=GetGamepadAxisMovement(0,GAMEPAD_AXIS_RIGHT_X);
                const float lookY=GetGamepadAxisMovement(0,GAMEPAD_AXIS_RIGHT_Y);
                if(std::abs(lookX)>0.16f) yaw+=lookX*2.2f*controlDt;
                if(std::abs(lookY)>0.16f) pitch=std::clamp(pitch-lookY*2.0f*controlDt,-MAX_PITCH,MAX_PITCH);
            }
        }

        const float forwardX=std::sin(yaw),forwardZ=-std::cos(yaw);
        const float rightX=std::cos(yaw),rightZ=std::sin(yaw);
        float mx=0,mz=0;
        if(captured && IsKeyDown(KEY_W)) { mx+=forwardX; mz+=forwardZ; }
        if(captured && IsKeyDown(KEY_S)) { mx-=forwardX; mz-=forwardZ; }
        if(captured && IsKeyDown(KEY_D)) { mx+=rightX; mz+=rightZ; }
        if(captured && IsKeyDown(KEY_A)) { mx-=rightX; mz-=rightZ; }
        if(captured && gamepad) {
            const float stickX=GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_X);
            const float stickY=GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_Y);
            if(std::abs(stickX)>0.16f) { mx+=rightX*stickX; mz+=rightZ*stickX; }
            if(std::abs(stickY)>0.16f) { mx-=forwardX*stickY; mz-=forwardZ*stickY; }
        }
        const float moveLength=std::sqrt(mx*mx+mz*mz);
        if(moveLength>1.0f) { mx/=moveLength; mz/=moveLength; }

        const float speed=swimming?2.35f:(crouching?2.5f:6.0f);
        const float targetVX=mx*speed;
        const float targetVZ=mz*speed;
        const float response=(moveLength>0.001f)
            ? (swimming?18.0f:(grounded?42.0f:12.0f))
            : (grounded?24.0f:3.5f);
        vx += std::clamp(targetVX-vx,-response*controlDt,response*controlDt);
        vz += std::clamp(targetVZ-vz,-response*controlDt,response*controlDt);

        // Keep the player centred in a block cell and resolve X/Z separately,
        // so sliding along walls does not pin the player against corners.
        const float nextX=std::clamp(px+vx*controlDt,0.5f,float(size)-0.5f);
        if(!player_collides(world,nextX,py,pz)) px=nextX;
        else if(grounded && !player_collides(world,nextX,py+BLOCK*0.6f,pz)) {
            px=nextX; py+=BLOCK*0.6f;
        } else vx=0.0f;

        const float nextZ=std::clamp(pz+vz*controlDt,0.5f,float(size)-0.5f);
        if(!player_collides(world,px,py,nextZ)) pz=nextZ;
        else if(grounded && !player_collides(world,px,py+BLOCK*0.6f,nextZ)) {
            pz=nextZ; py+=BLOCK*0.6f;
        } else vz=0.0f;

        const bool nowSwimming=player_in_water(world,px,py,pz);
        if(captured && grounded &&
           (IsKeyPressed(KEY_SPACE)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN)))) {
            vy=nowSwimming?5.0f:10.5f;
            grounded=false;
        }
        const float gravity=nowSwimming?6.0f:25.0f;
        vy-=gravity*controlDt;
        if(nowSwimming && captured && IsKeyDown(KEY_SPACE))
            vy=std::min(vy+22.0f*controlDt,5.0f);
        vy=std::max(vy,nowSwimming?-8.0f:-55.0f);
        const float nextY=py+vy*controlDt;
        if(player_collides(world,px,nextY,pz)) {
            // Resolve the contact instead of leaving the player at the last
            // arbitrary frame position (the old code caused tiny ground jitter).
            if(vy<0.0f) {
                float blocked=nextY, clear=py;
                for(int i=0;i<10;++i) {
                    const float mid=(blocked+clear)*0.5f;
                    if(player_collides(world,px,mid,pz)) blocked=mid;
                    else clear=mid;
                }
                py=clear;
                grounded=true;
            } else if(vy>0.0f) {
                float clear=py, blocked=nextY;
                for(int i=0;i<10;++i) {
                    const float mid=(clear+blocked)*0.5f;
                    if(player_collides(world,px,mid,pz)) blocked=mid;
                    else clear=mid;
                }
                py=clear;
                grounded=false;
            }
            vy=0.0f;
        } else {
            py=nextY;
            grounded=false;
        }
        if(py<0.0f) { py=0.0f; vy=0.0f; grounded=true; }

        if(captured) {
            for(int i=0;i<HOTBAR_SIZE;++i) if(IsKeyPressed(KEY_ONE+i)) selected=i;
            const float wheel=GetMouseWheelMove();
            if(wheel!=0) selected=(selected+(wheel>0?HOTBAR_SIZE-1:1))%HOTBAR_SIZE;
            if(gamepad) {
                // L1/R1 cycle the hotbar; analog L2/R2 are reserved for block actions.
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_LEFT_TRIGGER_1))
                    selected=(selected+HOTBAR_SIZE-1)%HOTBAR_SIZE;
                if(IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_TRIGGER_1))
                    selected=(selected+1)%HOTBAR_SIZE;
            }
        }
        if(inventoryOpen&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const int panelW=760,panelH=380,panelX=GetScreenWidth()/2-panelW/2,panelY=GetScreenHeight()/2-panelH/2;
            const Vector2 mouse=GetMousePosition();
            bool handled=false;
            for(int slot=0;slot<4;++slot) {
                const int sx=panelX+550+(slot%2)*58,sy=panelY+82+(slot/2)*58;
                if(mouse.x>=sx&&mouse.x<sx+48&&mouse.y>=sy&&mouse.y<sy+48) {
                    if(inventory[selected].count>0) {
                        if(craftingGrid[slot].count==0||craftingGrid[slot].id==inventory[selected].id) {
                            craftingGrid[slot].id=inventory[selected].id;
                            ++craftingGrid[slot].count; --inventory[selected].count;
                            if(inventory[selected].count==0) inventory[selected].id=BlockId::Air;
                        }
                    } else if(craftingGrid[slot].count>0) {
                        if(add_stack(inventory,craftingGrid[slot].id,1)) {
                            --craftingGrid[slot].count;
                            if(craftingGrid[slot].count==0) craftingGrid[slot].id=BlockId::Air;
                        }
                    }
                    handled=true; break;
                }
            }
            const int outX=panelX+690,outY=panelY+140;
            if(!handled&&mouse.x>=outX&&mouse.x<outX+48&&mouse.y>=outY&&mouse.y<outY+48) {
                const CraftResult result=match_recipe(craftingGrid);
                if(result.count>0&&add_stack(inventory,result.id,result.count)) {
                    for(auto& slot:craftingGrid) if(slot.count>0) {
                        --slot.count; if(slot.count==0) slot.id=BlockId::Air;
                    }
                }
                handled=true;
            }
            if(!handled) for(int i=0;i<INVENTORY_SIZE;++i) {
                const int col=i%9,row=i/9,sx=panelX+18+col*54,sy=panelY+55+row*58;
                if(mouse.x>=sx&&mouse.x<sx+48&&mouse.y>=sy&&mouse.y<sy+50) {
                    if(i<HOTBAR_SIZE) selected=i;
                    else std::swap(inventory[selected],inventory[i]);
                    break;
                }
            }
        }

        Camera3D camera{};
        const float eyeHeight=crouching?2.25f:EYE_HEIGHT;
        const Vector3 lookDirection=Vector3{std::sin(yaw)*std::cos(pitch),std::sin(pitch),-std::cos(yaw)*std::cos(pitch)};
        if(thirdPerson) {
            camera.position=Vector3{px*BLOCK-lookDirection.x*8.0f,py+eyeHeight+lookDirection.y*2.0f-lookDirection.y*8.0f,pz*BLOCK-lookDirection.z*8.0f};
            camera.target=Vector3{px*BLOCK,py+eyeHeight,pz*BLOCK};
        } else {
            camera.position=Vector3{px*BLOCK,py+eyeHeight,pz*BLOCK};
            camera.target=Vector3Add(camera.position,Vector3Scale(lookDirection,1.0f));
        }
        camera.up=Vector3{0,1,0}; camera.fovy=cameraFov; camera.projection=CAMERA_PERSPECTIVE;

        Vector3 hit{},previous{};
        const Ray centerRay{camera.position,Vector3Normalize(Vector3Subtract(camera.target,camera.position))};
        const bool hasTarget=ray_block(centerRay,world,hit,previous);
        const bool miningHeld=IsMouseButtonDown(MOUSE_BUTTON_LEFT)||
            (gamepad&&GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_TRIGGER)>0.2f);
        if(!paused&&!inventoryOpen&&!creativeOpen&&captured&&hasTarget&&miningHeld) {
            const int bx=int(hit.x),by=int(hit.y),bz=int(hit.z);
            const BlockId targetBlock=world.get_block(bx,by,bz);
            if(targetBlock!=BlockId::Air&&targetBlock!=BlockId::Bedrock&&targetBlock!=BlockId::Water) {
                if(bx!=miningX||by!=miningY||bz!=miningZ) {
                    miningX=bx; miningY=by; miningZ=bz; miningProgress=0.0f;
                }
                const float hardness=(targetBlock==BlockId::Dirt||targetBlock==BlockId::Sand||
                    targetBlock==BlockId::Gravel||targetBlock==BlockId::Leaves)?0.32f:
                    ((targetBlock==BlockId::Wood)?0.55f:0.85f);
                miningProgress=std::min(1.0f,miningProgress+dt/hardness);
                if(miningProgress>=1.0f && add_item(inventory,targetBlock)) {
                    const Vector3 center=block_center(bx,by,bz);
                    const Color particleColor=color_for(targetBlock);
                    for(int i=0;i<16;++i) {
                        const float fx=float((i*37)%11-5)*0.34f;
                        const float fy=float((i*19)%9)*0.30f+1.2f;
                        const float fz=float((i*23)%13-6)*0.30f;
                        breakParticles.push_back(BreakParticle{
                            Vector3{center.x+fx*0.35f,center.y+fy*0.25f,center.z+fz*0.35f},
                            Vector3{fx,fy,fz},0.0f,0.42f,particleColor});
                    }
                    world.set_block(bx,by,bz,BlockId::Air);
                    miningX=miningY=miningZ=-1; miningProgress=0.0f;
                }
            }
        } else {
            miningX=miningY=miningZ=-1;
            miningProgress=0.0f;
        }
        if(!paused&&!inventoryOpen&&!creativeOpen&&captured&&hasTarget&&(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)||(gamepad&&GetGamepadAxisMovement(0,GAMEPAD_AXIS_RIGHT_TRIGGER)>0.2f))) {
            const int bx=int(previous.x),by=int(previous.y),bz=int(previous.z);
            ItemStack& stack=inventory[selected];
            if(by>=1&&by<Chunk::Height&&stack.count>0&&stack.id!=BlockId::Air) {
                const float cx=(float(bx)+0.5f)*BLOCK,cz=(float(bz)+0.5f)*BLOCK;
                const bool overlaps=std::abs(cx-px*BLOCK)<1.0f&&std::abs(cz-pz*BLOCK)<1.0f&&float(by+1)*BLOCK>py&&float(by)*BLOCK<py+PLAYER_HEIGHT;
                if(!overlaps&&world.set_block(bx,by,bz,stack.id)) --stack.count;
            }
        }

        BeginDrawing();
        ClearBackground(Color{135,190,235,255});
        BeginMode3D(camera);
        const int minX=std::max(0,int(px)-renderRadius),maxX=std::min(size,int(px)+renderRadius+1);
        const int minZ=std::max(0,int(pz)-renderRadius),maxZ=std::min(size,int(pz)+renderRadius+1);
        std::vector<std::pair<BlockId,Vector3>> transparentBlocks;
        for(int z=minZ;z<maxZ;++z) for(int x=minX;x<maxX;++x) {
            const int minY=std::max(0,int(py/BLOCK)-verticalRadius),maxY=std::min(Chunk::Height,int(py/BLOCK)+verticalRadius+2);
            for(int y=minY;y<maxY;++y) {
                const BlockId id=world.get_block(x,y,z);
                if(id==BlockId::Air) continue;
                if(world.get_block(x,y+1,z)!=BlockId::Air && world.get_block(x,y+1,z)!=BlockId::Water &&
                   world.get_block(x+1,y,z)!=BlockId::Air && world.get_block(x-1,y,z)!=BlockId::Air &&
                   world.get_block(x,y,z+1)!=BlockId::Air && world.get_block(x,y,z-1)!=BlockId::Air) continue;
                const Vector3 center=block_center(x,y,z);
                if(id==BlockId::Water) transparentBlocks.emplace_back(id,center);
                else if(!draw_textured_block(blockAtlas,id,center)) DrawCube(center,BLOCK,BLOCK,BLOCK,color_for(id));
            }
        }
        const auto distanceSquared=[&](const Vector3& p) {
            const float dx=p.x-camera.position.x,dy=p.y-camera.position.y,dz=p.z-camera.position.z;
            return dx*dx+dy*dy+dz*dz;
        };
        std::sort(transparentBlocks.begin(),transparentBlocks.end(),
            [&](const auto& a,const auto& b){ return distanceSquared(a.second)>distanceSquared(b.second); });
        BeginBlendMode(BLEND_ALPHA);
        for(const auto& entry:transparentBlocks) {
            if(!draw_textured_block(blockAtlas,entry.first,entry.second))
                DrawCube(entry.second,BLOCK,BLOCK,BLOCK,color_for(entry.first));
        }
        EndBlendMode();
        if(hasTarget) {
            const Vector3 selectedCenter=block_center(int(hit.x),int(hit.y),int(hit.z));
            DrawCubeWires(selectedCenter,BLOCK+0.025f,BLOCK+0.025f,BLOCK+0.025f,YELLOW);
        }
        for(const BreakParticle& particle:breakParticles) {
            const float alpha=std::clamp(1.0f-particle.life/particle.maxLife,0.0f,1.0f);
            DrawCube(particle.position,0.16f,0.16f,0.16f,Fade(particle.color,alpha));
        }
        EndMode3D();
        DrawRectangle(0,0,GetScreenWidth(),38,Fade(BLACK,0.35f));
        DrawText("MINE C++ CLONE  |  SURVIVAL TEST",14,10,18,RAYWHITE);
        DrawText("World autosaves to world.mcc when you exit",14,48,14,RAYWHITE);
        DrawText("Move: WASD / left stick | Look: mouse / right stick | Jump: SPACE / A | Break: R2 / ZR | Place: L2 / ZL | Hotbar: L1/R1 | E: inventory + crafting | R: creative",14,GetScreenHeight()-28,13,RAYWHITE);
        if(miningProgress>0.0f) {
            const int w=180,x=GetScreenWidth()/2-w/2,y=GetScreenHeight()/2+28;
            DrawRectangle(x,y,w,8,Fade(BLACK,0.65f));
            DrawRectangle(x,y,int(float(w)*miningProgress),8,YELLOW);
            DrawRectangleLines(x,y,w,8,RAYWHITE);
        }
        DrawRectangle(GetScreenWidth()/2-1,GetScreenHeight()/2-8,2,16,RAYWHITE);
        DrawRectangle(GetScreenWidth()/2-8,GetScreenHeight()/2-1,16,2,RAYWHITE);
        const int barW=52,barX=GetScreenWidth()/2-(barW*HOTBAR_SIZE)/2,barY=GetScreenHeight()-100;
        for(int i=0;i<HOTBAR_SIZE;++i) {
            const ItemStack& stack=inventory[i];
            DrawRectangle(barX+i*barW,barY,48,56,Fade(BLACK,0.72f));
            DrawRectangleLines(barX+i*barW,barY,48,56,i==selected?YELLOW:GRAY);
            if(stack.count>0) { if((blockAtlas.texture.id != 0)) DrawTexturePro(blockAtlas.texture,block_atlas_rect(stack.id),Rectangle{float(barX+i*barW+10),float(barY+13),28,28},Vector2{0,0},0.0f,WHITE); else DrawRectangle(barX+i*barW+10,barY+13,28,28,color_for(stack.id)); }
            DrawText(TextFormat("%i",i+1),barX+i*barW+3,barY+3,11,RAYWHITE);
            if(stack.count>0) DrawText(TextFormat("%i",stack.count),barX+i*barW+29,barY+41,12,RAYWHITE);
        }
        if(creativeOpen) {
            const int panelW=560,panelH=330,panelX=GetScreenWidth()/2-panelW/2,panelY=GetScreenHeight()/2-panelH/2;
            DrawRectangle(panelX,panelY,panelW,panelH,Fade(Color{38,38,42,255},0.97f));
            DrawRectangleLines(panelX,panelY,panelW,panelH,RAYWHITE);
            DrawText("CREATIVE MODE",panelX+22,panelY+18,26,RAYWHITE);
            DrawText("Creative inventory is not implemented yet.",panelX+24,panelY+88,19,YELLOW);
            DrawText("This menu will contain the full block and item catalog.",panelX+24,panelY+126,16,LIGHTGRAY);
            DrawText("R: close",panelX+24,panelY+282,16,RAYWHITE);
        }
        if(paused) {
            DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.78f));
            DrawText("GAME MENU",GetScreenWidth()/2-105,GetScreenHeight()/2-190,34,RAYWHITE);
            if(pauseOptionsOpen) {
                DrawText("SETTINGS",GetScreenWidth()/2-82,GetScreenHeight()/2-115,28,YELLOW);
                DrawText(TextFormat("Field of view: %.0f",cameraFov),GetScreenWidth()/2-115,GetScreenHeight()/2-42,21,RAYWHITE);
                DrawText("UP / DOWN: change FOV",GetScreenWidth()/2-120,GetScreenHeight()/2+12,17,LIGHTGRAY);
                DrawText("BACKSPACE / B: return",GetScreenWidth()/2-128,GetScreenHeight()/2+58,17,LIGHTGRAY);
                DrawText("PLUS / START: resume game",GetScreenWidth()/2-145,GetScreenHeight()/2+105,16,LIGHTGRAY);
            } else {
                const char* pauseLabels[3]={"Back to Game","Settings","Save & Quit to Main Menu"};
                const int bw=300,bh=48,gap=12,bx=GetScreenWidth()/2-bw/2,by=GetScreenHeight()/2-5;
                const Vector2 mouse=GetMousePosition();
                for(int i=0;i<3;++i) {
                    const int y=by+i*(bh+gap);
                    const Rectangle r{float(bx),float(y),float(bw),float(bh)};
                    const bool hover=CheckCollisionPointRec(mouse,r);
                    const bool active=(pauseSelection==i)||hover;
                    DrawRectangleRec(r,active?Color{65,65,65,250}:Color{30,30,30,240});
                    DrawRectangleLinesEx(r,2,active?YELLOW:GRAY);
                    DrawText(pauseLabels[i],bx+(bw-MeasureText(pauseLabels[i],19))/2,y+13,19,RAYWHITE);
                }
                DrawText("PLUS / START / ESC: resume",GetScreenWidth()/2-145,by+3*(bh+gap)+8,16,LIGHTGRAY);
            }
        }
        if(inventoryOpen) {
            const int panelW=760,panelH=380,panelX=GetScreenWidth()/2-panelW/2,panelY=GetScreenHeight()/2-panelH/2;
            DrawRectangle(panelX,panelY,panelW,panelH,Fade(DARKGRAY,0.97f));
            DrawRectangleLines(panelX,panelY,panelW,panelH,RAYWHITE);
            DrawText("INVENTORY",panelX+18,panelY+14,22,RAYWHITE);
            DrawText("2x2 CRAFTING",panelX+540,panelY+48,18,YELLOW);
            for(int i=0;i<INVENTORY_SIZE;++i) {
                const int col=i%9,row=i/9,sx=panelX+18+col*54,sy=panelY+55+row*58;
                DrawRectangle(sx,sy,48,50,Fade(BLACK,0.7f));
                DrawRectangleLines(sx,sy,48,50,i==selected?YELLOW:GRAY);
                if(inventory[i].count>0) {
                    if(blockAtlas.texture.id!=0) DrawTexturePro(blockAtlas.texture,block_atlas_rect(inventory[i].id),Rectangle{float(sx+10),float(sy+8),28,28},Vector2{0,0},0.0f,WHITE);
                    else DrawRectangle(sx+10,sy+8,28,28,color_for(inventory[i].id));
                    DrawText(TextFormat("%i",inventory[i].count),sx+27,sy+34,12,RAYWHITE);
                }
            }
            for(int i=0;i<4;++i) {
                const int sx=panelX+550+(i%2)*58,sy=panelY+82+(i/2)*58;
                DrawRectangle(sx,sy,48,48,Fade(BLACK,0.75f)); DrawRectangleLines(sx,sy,48,48,GRAY);
                if(craftingGrid[i].count>0) {
                    if(blockAtlas.texture.id!=0) DrawTexturePro(blockAtlas.texture,block_atlas_rect(craftingGrid[i].id),Rectangle{float(sx+8),float(sy+7),32,32},Vector2{0,0},0.0f,WHITE);
                    else DrawRectangle(sx+8,sy+7,32,32,color_for(craftingGrid[i].id));
                    DrawText(TextFormat("%i",craftingGrid[i].count),sx+29,sy+33,11,RAYWHITE);
                }
            }
            DrawText("=>",panelX+665,panelY+150,22,RAYWHITE);
            const int outX=panelX+690,outY=panelY+140;
            DrawRectangle(outX,outY,48,48,Fade(BLACK,0.75f)); DrawRectangleLines(outX,outY,48,48,YELLOW);
            const CraftResult result=match_recipe(craftingGrid);
            if(result.count>0) {
                if(blockAtlas.texture.id!=0) DrawTexturePro(blockAtlas.texture,block_atlas_rect(result.id),Rectangle{float(outX+8),float(outY+7),32,32},Vector2{0,0},0.0f,WHITE);
                else DrawRectangle(outX+8,outY+7,32,32,color_for(result.id));
                DrawText(TextFormat("%i",result.count),outX+29,outY+33,11,RAYWHITE);
            }
            DrawText("Click a hotbar stack, then a crafting slot. Click output to craft.",panelX+535,panelY+225,13,RAYWHITE);
            DrawText("Recipes: 1 log = 4 planks | 2 vertical planks = 4 sticks | 4 planks = crafting table",panelX+535,panelY+250,12,LIGHTGRAY);
            DrawText("Hotbar: keys 1-9 | E: close",panelX+18,panelY+330,14,RAYWHITE);
        }
        DrawText(TextFormat("XYZ %.1f / %.1f / %.1f",px,py/BLOCK,pz),14,48,16,RAYWHITE);
        if(!captured) DrawText("Cursor released - press ESC to resume",GetScreenWidth()/2-170,GetScreenHeight()/2+24,18,YELLOW);
        DrawFPS(GetScreenWidth()-90,12);
        EndDrawing();
    }
    { std::ofstream saveFile("world.mcc",std::ios::binary|std::ios::trunc); if(saveFile) world.save(saveFile); }
    EnableCursor();
    unload_block_atlas(blockAtlas);
    CloseWindow();
    return 0;
}
}
