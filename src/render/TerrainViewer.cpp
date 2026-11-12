#include "TerrainViewer.hpp"

#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mc::render {
namespace {
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
float terrain(int x,int z,std::uint32_t seed) {
    float n=0.0f,amp=1.0f,freq=0.012f,total=0.0f;
    for(int octave=0;octave<5;++octave) {
        n+=noise(float(x)*freq,float(z)*freq,seed+std::uint32_t(octave)*1013u)*amp;
        total+=amp; amp*=0.5f; freq*=2.0f;
    }
    const float continental=noise(float(x)*0.003f,float(z)*0.003f,seed^0x9e3779b9u);
    return std::clamp(62.0f+(n/total)*25.0f+continental*13.0f,4.0f,120.0f);
}
}
int run_terrain_viewer(int map_size,std::uint32_t seed) {
    const int size=std::clamp(map_size,64,512);
    Image heightmap=GenImageColor(size,size,WHITE);
    Image colorMap=GenImageColor(size,size,WHITE);
    for(int z=0;z<size;++z) for(int x=0;x<size;++x) {
        const float h=terrain(x,z,seed);
        const unsigned char gray=static_cast<unsigned char>(std::clamp(h/120.0f*255.0f,0.0f,255.0f));
        Color color;
        if(h<53.0f) color=Color{35,75,150,255};
        else if(h<69.0f) color=Color{194,178,128,255};
        else if(h<91.0f) color=Color{65,145,62,255};
        else color=Color{135,135,140,255};
        ImageDrawPixel(&heightmap,x,z,Color{gray,gray,gray,255});
        ImageDrawPixel(&colorMap,x,z,color);
    }
    Mesh mesh=GenMeshHeightmap(heightmap,Vector3{float(size),36.0f,float(size)});
    Model model=LoadModelFromMesh(mesh);
    Texture2D terrainTexture=LoadTextureFromImage(colorMap);
    SetMaterialTexture(&model.materials[0],MATERIAL_MAP_DIFFUSE,terrainTexture);
    UnloadImage(heightmap);
    UnloadImage(colorMap);

    InitWindow(1280,720,"Mine C++ Clone - 3D terrain preview");
    SetTargetFPS(60);
    Camera3D camera{};
    camera.position=Vector3{float(size)*0.5f,58.0f,float(size)*0.85f};
    camera.target=Vector3{float(size)*0.5f,12.0f,float(size)*0.5f};
    camera.up=Vector3{0.0f,1.0f,0.0f};
    camera.fovy=65.0f;
    camera.projection=CAMERA_PERSPECTIVE;
    DisableCursor();
    while(!WindowShouldClose()) {
        UpdateCamera(&camera,CAMERA_FIRST_PERSON);
        BeginDrawing();
        ClearBackground(Color{135,190,235,255});
        BeginMode3D(camera);
        DrawModel(model,Vector3{-float(size)*0.5f,0.0f,-float(size)*0.5f},1.0f,WHITE);
        DrawGrid(40,4.0f);
        EndMode3D();
        DrawRectangle(12,12,430,72,Fade(BLACK,0.55f));
        DrawText("MINE C++ CLONE - TERRAIN PREVIEW",24,22,18,RAYWHITE);
        DrawText("WASD: move | Mouse: look | ESC: exit",24,48,16,RAYWHITE);
        DrawFPS(GetScreenWidth()-90,12);
        EndDrawing();
    }
    EnableCursor();
    UnloadTexture(terrainTexture);
    UnloadModel(model);
    CloseWindow();
    return 0;
}
}
