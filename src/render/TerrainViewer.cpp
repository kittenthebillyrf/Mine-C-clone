#include "TerrainViewer.hpp"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace mc::render {
namespace {
constexpr float BLOCK_SIZE=2.0f;
constexpr float SEA_LEVEL=52.0f;
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
    const int ix=int(std::floor(x)), iz=int(std::floor(z));
    const float fx=fade(x-float(ix)), fz=fade(z-float(iz));
    return lerp(lerp(lattice(ix,iz,seed),lattice(ix+1,iz,seed),fx),
                  lerp(lattice(ix,iz+1,seed),lattice(ix+1,iz+1,seed),fx),fz);
}
float fbm(float x,float z,std::uint32_t seed,int octaves,float frequency) {
    float value=0.0f,amplitude=1.0f,total=0.0f;
    for(int i=0;i<octaves;++i) {
        value+=noise(x*frequency,z*frequency,seed+std::uint32_t(i)*1013u)*amplitude;
        total+=amplitude; amplitude*=0.5f; frequency*=2.0f;
    }
    return value/total;
}
int terrain_height(int x,int z,std::uint32_t seed) {
    const float continent=fbm(float(x),float(z),seed^0x9e3779b9u,4,0.0035f);
    const float hills=fbm(float(x),float(z),seed,5,0.018f);
    const float ridges=1.0f-std::abs(fbm(float(x),float(z),seed^0x68bc21ebu,3,0.009f));
    const float height=48.0f+continent*18.0f+hills*17.0f+std::pow(ridges,3.0f)*13.0f;
    return std::clamp(int(std::round(height)),5,105);
}
Color surface_color(int h,int x,int z,std::uint32_t seed) {
    const float temperature=fbm(float(x),float(z),seed^0x1234abcdu,2,0.006f);
    const float moisture=fbm(float(x),float(z),seed^0x76543210u,2,0.008f);
    if(h<SEA_LEVEL+2) return Color{194,178,128,255}; // sand
    if(h>83) return Color{125,125,130,255};          // stone peaks
    if(temperature>0.35f && moisture< -0.05f) return Color{190,166,105,255}; // dry biome
    if(temperature< -0.30f) return Color{105,145,100,255}; // cold biome
    return Color{65,145,62,255};                     // grass
}
struct Column { int height; Color color; bool tree; };
}
int run_terrain_viewer(int map_size,std::uint32_t seed) {
    const int size=std::clamp(map_size,64,192);
    std::vector<Column> columns(std::size_t(size)*size);
    for(int z=0;z<size;++z) for(int x=0;x<size;++x) {
        const int h=terrain_height(x,z,seed);
        const float forest=fbm(float(x),float(z),seed^0xabcdef01u,2,0.035f);
        const bool tree=(h>SEA_LEVEL+3 && h<78 && forest>0.38f &&
                         (hash2(x,z,seed^0x55aa55aau)%100u)<9u);
        columns[std::size_t(z)*size+x]={h,surface_color(h,x,z,seed),tree};
    }

    InitWindow(1280,720,"Mine C++ Clone - voxel terrain");
    SetTargetFPS(60);
    Camera3D camera{};
    camera.position=Vector3{float(size)*BLOCK_SIZE*0.5f,145.0f,float(size)*BLOCK_SIZE*0.85f};
    camera.target=Vector3{float(size)*BLOCK_SIZE*0.5f,35.0f,float(size)*BLOCK_SIZE*0.5f};
    camera.up=Vector3{0.0f,1.0f,0.0f};
    camera.fovy=65.0f;
    camera.projection=CAMERA_PERSPECTIVE;
    DisableCursor();

    while(!WindowShouldClose()) {
        UpdateCamera(&camera,CAMERA_FIRST_PERSON);
        BeginDrawing();
        ClearBackground(Color{135,190,235,255});
        BeginMode3D(camera);
        for(int z=0;z<size;++z) for(int x=0;x<size;++x) {
            const Column& c=columns[std::size_t(z)*size+x];
            const float wx=float(x)*BLOCK_SIZE, wz=float(z)*BLOCK_SIZE;
            const float groundHeight=float(c.height)*BLOCK_SIZE;
            DrawCube(Vector3{wx,groundHeight*0.5f,wz},
                     BLOCK_SIZE,groundHeight,BLOCK_SIZE,c.color);
            DrawCubeWires(Vector3{wx,groundHeight*0.5f,wz},
                          BLOCK_SIZE,groundHeight,BLOCK_SIZE,Fade(BLACK,0.035f));
            if(c.height<SEA_LEVEL) {
                const float waterHeight=(SEA_LEVEL-float(c.height))*BLOCK_SIZE;
                DrawCube(Vector3{wx,groundHeight+waterHeight*0.5f,wz},
                         BLOCK_SIZE,waterHeight,BLOCK_SIZE,Color{45,105,190,170});
            }
            if(c.tree) {
                const float top=groundHeight;
                DrawCube(Vector3{wx,top+5.0f,wz},0.9f,10.0f,0.9f,Color{105,73,43,255});
                DrawCube(Vector3{wx,top+12.0f,wz},5.2f,6.0f,5.2f,Color{42,115,48,255});
                DrawCube(Vector3{wx,top+15.0f,wz},3.4f,4.0f,3.4f,Color{50,130,54,255});
            }
        }
        EndMode3D();
        DrawRectangle(12,12,520,92,Fade(BLACK,0.62f));
        DrawText("MINE C++ CLONE - BLOCK TERRAIN",24,22,20,RAYWHITE);
        DrawText("Large blocks: 2x2x2 | WASD: move | Mouse: look | ESC: exit",24,50,16,RAYWHITE);
        DrawText(TextFormat("World: %i x %i | Seed: %u | Water level: %.0f",size,size,seed,SEA_LEVEL),24,75,15,RAYWHITE);
        DrawFPS(GetScreenWidth()-90,12);
        EndDrawing();
    }
    EnableCursor();
    CloseWindow();
    return 0;
}
}
