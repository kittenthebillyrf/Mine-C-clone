#include "TerrainViewer.hpp"
#include "raylib.h"
#include "raymath.h"
#include "../world/World.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>

namespace mc::render {
namespace {
constexpr float BLOCK=2.0f;
constexpr float PLAYER_HEIGHT=3.6f;
constexpr float EYE_HEIGHT=3.15f;
constexpr float REACH=7.0f;
constexpr int HOTBAR_SIZE=9;
constexpr int INVENTORY_SIZE=36;
struct ItemStack { mc::world::BlockId id=mc::world::BlockId::Air; int count=0; };
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
        default:return BLANK;
    }
}
Texture2D make_block_texture(mc::world::BlockId id) {
    Image image=GenImageColor(16,16,WHITE);
    const Color base=color_for(id);
    for(int y=0;y<16;++y) for(int x=0;x<16;++x) {
        const std::uint32_t h=hash2(x,y,0x51f15e5u+static_cast<std::uint32_t>(id)*977u);
        const int variation=int(h%37u)-18;
        Color c=base;
        c.r=static_cast<unsigned char>(std::clamp(int(base.r)+variation,0,255));
        c.g=static_cast<unsigned char>(std::clamp(int(base.g)+variation,0,255));
        c.b=static_cast<unsigned char>(std::clamp(int(base.b)+variation,0,255));
        if(id==mc::world::BlockId::Grass && y<4) c=Color{75,174,58,255};
        if(id==mc::world::BlockId::Water) c=Color{static_cast<unsigned char>(35+int(h%18u)),static_cast<unsigned char>(105+int(h%35u)),static_cast<unsigned char>(190+int(h%45u)),210};
        ImageDrawPixel(&image,x,y,c);
    }
    Texture2D texture=LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture,TEXTURE_FILTER_POINT);
    return texture;
}
void draw_block(const Texture2D* textures,mc::world::BlockId id,Vector3 center) {
    const auto index=static_cast<unsigned int>(id);
    if(index<7 && textures[index].id!=0) DrawCubeTexture(textures[index],center,BLOCK,BLOCK,BLOCK,WHITE);
    else DrawCube(center,BLOCK,BLOCK,BLOCK,color_for(id));
}
Vector3 block_center(int x,int y,int z) { return Vector3{(float(x)+0.5f)*BLOCK,(float(y)+0.5f)*BLOCK,(float(z)+0.5f)*BLOCK}; }
int surface_y(const mc::world::World& world,int x,int z) {
    for(int y=mc::world::Chunk::Height-1;y>=0;--y) {
        const auto id=world.get_block(x,y,z);
        if(id!=mc::world::BlockId::Air&&id!=mc::world::BlockId::Water) return y+1;
    }
    return 1;
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
    for(int z=0;z<size;++z) for(int x=0;x<size;++x) {
        const int h=height_at(x,z,seed);
        const bool sandy=h<=11;
        for(int y=0;y<h;++y) {
            BlockId id=(y==0)?BlockId::Bedrock:(y<h-4?BlockId::Stone:(y<h-1?BlockId::Dirt:(sandy?BlockId::Sand:BlockId::Grass)));
            world.set_block(x,y,z,id);
        }
        if(h<10) for(int y=h;y<11;++y) world.set_block(x,y,z,BlockId::Water);
    }

    { std::ifstream saveFile("world.mcc",std::ios::binary); if(saveFile) world.load(saveFile); }

    InitWindow(1280,720,"Mine C++ Clone - playable voxel prototype");
    SetTargetFPS(120);
    DisableCursor();
    Texture2D blockTextures[7]{};
    for(unsigned int i=1;i<7;++i) blockTextures[i]=make_block_texture(static_cast<BlockId>(i));
    float px=float(size)*0.5f,pz=float(size)*0.5f;
    float py=float(surface_y(world,int(px),int(pz)))*BLOCK;
    float vy=0.0f,yaw=0.0f,pitch=-0.18f;
    bool grounded=true;
    int selected=0;
    ItemStack inventory[INVENTORY_SIZE]{};
    const BlockId startingItems[5]={BlockId::Grass,BlockId::Dirt,BlockId::Stone,BlockId::Sand,BlockId::Water};
    for(int i=0;i<5;++i) inventory[i]={startingItems[i],32};
    bool inventoryOpen=false;
    bool creativeOpen=false;
    bool paused=false;
    bool thirdPerson=false;
    bool captured=true;
    bool inMenu=true;
    bool optionsOpen=false;
    bool multiplayerOpen=false;
    bool requestQuit=false;
    float menuYaw=0.0f;
    while(!WindowShouldClose()) {
        const float dt=std::min(GetFrameTime(),0.04f);
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
                            draw_block(blockTextures,id,block_center(x,y,z));
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
            const bool menuConfirm=IsKeyPressed(KEY_ENTER)||(menuPad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
            const bool menuBack=IsKeyPressed(KEY_ESCAPE)||(menuPad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
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
            paused=!paused;
            inventoryOpen=false; creativeOpen=false;
            captured=!paused;
            if(captured) DisableCursor(); else EnableCursor();
        }
        if(!paused && (IsKeyPressed(KEY_E)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)))) {
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
        const float controlDt=(paused||inventoryOpen)?0.0f:dt;
        if(captured) {
            const Vector2 md=GetMouseDelta();
            yaw+=md.x*0.0025f;
            pitch=std::clamp(pitch-md.y*0.0025f,-1.45f,1.45f);
            if(gamepad) {
                const float lookX=GetGamepadAxisMovement(0,GAMEPAD_AXIS_RIGHT_X);
                const float lookY=GetGamepadAxisMovement(0,GAMEPAD_AXIS_RIGHT_Y);
                if(std::abs(lookX)>0.16f) yaw+=lookX*2.2f*dt;
                if(std::abs(lookY)>0.16f) pitch=std::clamp(pitch-lookY*2.0f*dt,-1.45f,1.45f);
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
        const float len=std::sqrt(mx*mx+mz*mz);
        if(len>0) { mx/=len; mz/=len; }
        const bool crouching=IsKeyDown(KEY_LEFT_SHIFT);
        const float speed=crouching?2.5f:6.0f;
        const float nx=std::clamp(px+mx*speed*dt,1.0f,float(size-2));
        const float nz=std::clamp(pz+mz*speed*dt,1.0f,float(size-2));
        const int groundX=int(std::floor(nx)),groundZ=int(std::floor(nz));
        const float ground=float(surface_y(world,groundX,groundZ))*BLOCK;
        if(ground<=py+BLOCK*1.1f || !grounded) { px=nx; pz=nz; }
        if(captured && grounded && (IsKeyPressed(KEY_SPACE)||(gamepad&&IsGamepadButtonPressed(0,GAMEPAD_BUTTON_RIGHT_FACE_DOWN)))) { vy=10.5f; grounded=false; }
        vy-=25.0f*dt; py+=vy*dt;
        const float floorY=float(surface_y(world,int(px),int(pz)))*BLOCK;
        if(py<=floorY) { py=floorY; vy=0; grounded=true; }

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
        const float eyeHeight=crouching?2.25f:EYE_HEIGHT;
        const Vector3 lookDirection=Vector3{std::sin(yaw)*std::cos(pitch),std::sin(pitch),-std::cos(yaw)*std::cos(pitch)};
        if(thirdPerson) {
            camera.position=Vector3{px*BLOCK-lookDirection.x*8.0f,py+eyeHeight+lookDirection.y*2.0f-lookDirection.y*8.0f,pz*BLOCK-lookDirection.z*8.0f};
            camera.target=Vector3{px*BLOCK,py+eyeHeight,pz*BLOCK};
        } else {
            camera.position=Vector3{px*BLOCK,py+eyeHeight,pz*BLOCK};
            camera.target=Vector3Add(camera.position,Vector3Scale(lookDirection,1.0f));
        }
        camera.up=Vector3{0,1,0}; camera.fovy=70; camera.projection=CAMERA_PERSPECTIVE;

        Vector3 hit{},previous{};
        const bool hasTarget=ray_block(GetScreenToWorldRay(Vector2{float(GetScreenWidth())*0.5f,float(GetScreenHeight())*0.5f},camera),world,hit,previous);
        if(!paused&&!inventoryOpen&&!creativeOpen&&captured&&hasTarget&&(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)||(gamepad&&GetGamepadAxisMovement(0,GAMEPAD_AXIS_LEFT_TRIGGER)>0.2f))) {
            const int bx=int(hit.x),by=int(hit.y),bz=int(hit.z);
            const BlockId broken=world.get_block(bx,by,bz);
            if(broken!=BlockId::Air&&broken!=BlockId::Bedrock&&broken!=BlockId::Water) {
                // Never delete a block if its drop cannot fit in the inventory.
                if(add_item(inventory,broken)) world.set_block(bx,by,bz,BlockId::Air);
            }
        }
        if(!paused&&!inventoryOpen&&captured&&hasTarget&&(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)||(gamepad&&GetGamepadAxisMovement(0,GAMEPAD_AXIS_RIGHT_TRIGGER)>0.2f))) {
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
        const int minX=std::max(0,int(px)-14),maxX=std::min(size,int(px)+15);
        const int minZ=std::max(0,int(pz)-14),maxZ=std::min(size,int(pz)+15);
        for(int z=minZ;z<maxZ;++z) for(int x=minX;x<maxX;++x) {
            const int minY=std::max(0,int(py/BLOCK)-18),maxY=std::min(Chunk::Height,int(py/BLOCK)+22);
            for(int y=minY;y<maxY;++y) {
                const BlockId id=world.get_block(x,y,z);
                if(id==BlockId::Air) continue;
                if(world.get_block(x,y+1,z)!=BlockId::Air && world.get_block(x,y+1,z)!=BlockId::Water &&
                   world.get_block(x+1,y,z)!=BlockId::Air && world.get_block(x-1,y,z)!=BlockId::Air &&
                   world.get_block(x,y,z+1)!=BlockId::Air && world.get_block(x,y,z-1)!=BlockId::Air) continue;
                draw_block(blockTextures,id,block_center(x,y,z));
            }
        }
        EndMode3D();
        DrawRectangle(0,0,GetScreenWidth(),38,Fade(BLACK,0.35f));
        DrawText("MINE C++ CLONE  |  SURVIVAL TEST",14,10,18,RAYWHITE);
        DrawText("World autosaves to world.mcc when you exit",14,48,14,RAYWHITE);
        DrawText("Move: WASD / left stick | Look: mouse / right stick | Jump: SPACE / B | Break: ZR | Place: ZL | Hotbar: L/R | A: inventory | X: creative | +: pause",14,GetScreenHeight()-28,13,RAYWHITE);
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
            const int panelW=560,panelH=330,panelX=GetScreenWidth()/2-panelW/2,panelY=GetScreenHeight()/2-panelH/2;
            DrawRectangle(panelX,panelY,panelW,panelH,Fade(Color{38,38,42,255},0.97f));
            DrawRectangleLines(panelX,panelY,panelW,panelH,RAYWHITE);
            DrawText("CREATIVE MODE",panelX+22,panelY+18,26,RAYWHITE);
            DrawText("Creative inventory is not implemented yet.",panelX+24,panelY+88,19,YELLOW);
            DrawText("This menu will contain the full block and item catalog.",panelX+24,panelY+126,16,LIGHTGRAY);
            DrawText("R: close",panelX+24,panelY+282,16,RAYWHITE);
        }
        if(paused) {
            DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,0.68f));
            DrawText("PAUSED",GetScreenWidth()/2-75,GetScreenHeight()/2-75,36,RAYWHITE);
            DrawText("Press + / START / ESC to resume",GetScreenWidth()/2-180,GetScreenHeight()/2-18,20,RAYWHITE);
        }
        if(inventoryOpen) {
            const int panelW=520,panelH=310,panelX=GetScreenWidth()/2-panelW/2,panelY=GetScreenHeight()/2-panelH/2;
            DrawRectangle(panelX,panelY,panelW,panelH,Fade(DARKGRAY,0.96f));
            DrawRectangleLines(panelX,panelY,panelW,panelH,RAYWHITE);
            DrawText("INVENTORY - E / Triangle to close",panelX+18,panelY+14,22,RAYWHITE);
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
            DrawText("Hotbar: keys 1-9 | R: creative menu",panelX+18,panelY+286,14,RAYWHITE);
        }
        DrawText(TextFormat("XYZ %.1f / %.1f / %.1f",px,py/BLOCK,pz),14,48,16,RAYWHITE);
        if(!captured) DrawText("Cursor released - press ESC to resume",GetScreenWidth()/2-170,GetScreenHeight()/2+24,18,YELLOW);
        DrawFPS(GetScreenWidth()-90,12);
        EndDrawing();
    }
    { std::ofstream saveFile("world.mcc",std::ios::binary|std::ios::trunc); if(saveFile) world.save(saveFile); }
    for(unsigned int i=1;i<7;++i) if(blockTextures[i].id!=0) UnloadTexture(blockTextures[i]);
    for(unsigned int i=1;i<7;++i) if(blockTextures[i].id!=0) UnloadTexture(blockTextures[i]);
    EnableCursor();
    CloseWindow();
    return 0;
}
}
