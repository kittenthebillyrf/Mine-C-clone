#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../world/Block.hpp"
#include <string>
#include <vector>

namespace mc::render {
inline Image make_block_atlas_image() {
    Image image=GenImageColor(64,64,Color{0,0,0,255});
    auto put=[&](int tile,int x,int y,Color c) { ImageDrawPixel(&image,(tile%4)*16+x,(tile/4)*16+y,c); };
    auto noise=[](int tile,int x,int y)->int {
        unsigned v=static_cast<unsigned>(tile*73856093u)^static_cast<unsigned>(x*19349663u)^static_cast<unsigned>(y*83492791u);
        v^=v>>13; v*=1274126177u; v^=v>>16;
        return int(v%25u)-12;
    };
    const Color base[16]={
        {134,93,58,255}, {105,174,65,255}, {119,82,48,255}, {58,133,53,255},
        {126,128,132,255}, {218,204,151,255}, {48,111,191,190}, {61,61,64,255},
        {125,87,49,255}, {157,116,69,255}, {133,130,124,255}, {83,83,86,255},
        {190,143,112,255}, {174,132,78,255}, {173,133,83,255}, {148,102,58,255}
    };
    for(int tile=0;tile<16;++tile) for(int y=0;y<16;++y) for(int x=0;x<16;++x) {
        Color c=base[tile]; const int n=noise(tile,x,y);
        c.r=static_cast<unsigned char>(std::clamp(int(c.r)+n,0,255));
        c.g=static_cast<unsigned char>(std::clamp(int(c.g)+n,0,255));
        c.b=static_cast<unsigned char>(std::clamp(int(c.b)+n,0,255));
        if(tile==1 && (x==0||y==0||x==15||y==15)) c=Color{91,153,52,255};
        if(tile==2 && y<3) c=Color{92,153,54,255};
        if(tile==2 && y>=3) c=Color{132,91,55,255};
        if(tile==3 && ((x*7+y*11)%13<4)) c=Color{42,111,44,255};
        if(tile==6) { c.a=190; if((x+y)%7==0) c=Color{103,164,218,185}; }
        if(tile==8 && (x%4==0||y%4==0)) c=Color{91,62,39,255};
        if(tile==9 && (x%5==0||y%5==0)) c=Color{111,75,44,255};
        if(tile==10 && (x+y)%9==0) c=Color{179,176,166,255};
        if(tile==11 && (x*3+y*5)%17<4) c=Color{39,39,42,255};
        if(tile==12 && (x*5+y*3)%19<4) c=Color{224,158,115,255};
        if(tile==13 && (y%4==0)) c=Color{139,99,63,255};
        if(tile==14 && (x%4==0||y%4==0)) c=Color{124,87,55,255};
        if(tile==15 && (x<3||x>12||y<3||y>12)) c=Color{91,59,35,255};
        put(tile,x,y,c);
    }
    return image;
}
inline int block_atlas_tile(mc::world::BlockId id) {
    using mc::world::BlockId;
    switch(id) {
        case BlockId::Stone:return 4;
        case BlockId::Grass:return 2;
        case BlockId::Dirt:return 0;
        case BlockId::Sand:return 5;
        case BlockId::Water:return 6;
        case BlockId::Bedrock:return 7;
        case BlockId::Wood:return 8;
        case BlockId::Leaves:return 3;
        case BlockId::Gravel:return 10;
        case BlockId::CoalOre:return 11;
        case BlockId::IronOre:return 12;
        case BlockId::Planks:return 13;
        case BlockId::Stick:return 14;
        case BlockId::CraftingTable:return 15;
        default:return 0;
    }
}
struct BlockAtlasAssets {
    Texture2D texture{};
    Material material{};
    Mesh meshes[16]{};
};

// Build a cube with independent UVs for every face. The previous implementation
// remapped the generated cube UVs as if all six faces shared one continuous atlas
// rectangle; that caused seams and incorrect sampling at tile boundaries.
inline Mesh make_block_mesh(int sideTile, int topTile, int bottomTile) {
    static constexpr float faceVertices[6][12] = {
        // +Z, -Z, +X, -X, +Y, -Y; four vertices per face, outward winding.
        {-1,-1, 1,  1,-1, 1,  1, 1, 1, -1, 1, 1},
        { 1,-1,-1, -1,-1,-1, -1, 1,-1,  1, 1,-1},
        { 1,-1, 1,  1,-1,-1,  1, 1,-1,  1, 1, 1},
        {-1,-1,-1, -1,-1, 1, -1, 1, 1, -1, 1,-1},
        {-1, 1, 1,  1, 1, 1,  1, 1,-1, -1, 1,-1},
        {-1,-1,-1,  1,-1,-1,  1,-1, 1, -1,-1, 1}
    };
    static constexpr float faceNormals[6][3] = {
        {0,0,1}, {0,0,-1}, {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}
    };
    static constexpr unsigned short faceIndices[6] = {0,1,2, 0,2,3};

    Mesh mesh{};
    mesh.vertexCount = 24;
    mesh.triangleCount = 12;
    mesh.vertices = static_cast<float*>(MemAlloc(sizeof(float) * 24 * 3));
    mesh.texcoords = static_cast<float*>(MemAlloc(sizeof(float) * 24 * 2));
    mesh.normals = static_cast<float*>(MemAlloc(sizeof(float) * 24 * 3));
    mesh.indices = static_cast<unsigned short*>(MemAlloc(sizeof(unsigned short) * 36));
    if (!mesh.vertices || !mesh.texcoords || !mesh.normals || !mesh.indices) {
        if (mesh.vertices) MemFree(mesh.vertices);
        if (mesh.texcoords) MemFree(mesh.texcoords);
        if (mesh.normals) MemFree(mesh.normals);
        if (mesh.indices) MemFree(mesh.indices);
        return Mesh{};
    }

    for (int face = 0; face < 6; ++face) {
        const int tile = (face == 4) ? topTile : ((face == 5) ? bottomTile : sideTile);
        const float tileX = float(tile % 4) * 16.0f;
        const float tileY = float(tile / 4) * 16.0f;
        // Half-texel inset prevents neighbouring atlas tiles bleeding into a face.
        const float u0 = (tileX + 0.5f) / 64.0f;
        const float u1 = (tileX + 15.5f) / 64.0f;
        const float v0 = 1.0f - (tileY + 15.5f) / 64.0f;
        const float v1 = 1.0f - (tileY + 0.5f) / 64.0f;
        const float uv[4][2] = {{u0,v0}, {u1,v0}, {u1,v1}, {u0,v1}};

        for (int corner = 0; corner < 4; ++corner) {
            const int vertex = face * 4 + corner;
            mesh.vertices[vertex*3+0] = faceVertices[face][corner*3+0];
            mesh.vertices[vertex*3+1] = faceVertices[face][corner*3+1];
            mesh.vertices[vertex*3+2] = faceVertices[face][corner*3+2];
            mesh.texcoords[vertex*2+0] = uv[corner][0];
            mesh.texcoords[vertex*2+1] = uv[corner][1];
            mesh.normals[vertex*3+0] = faceNormals[face][0];
            mesh.normals[vertex*3+1] = faceNormals[face][1];
            mesh.normals[vertex*3+2] = faceNormals[face][2];
        }
        const int indexBase = face * 6;
        for (int i = 0; i < 6; ++i)
            mesh.indices[indexBase+i] = static_cast<unsigned short>(face*4 + faceIndices[i]);
    }
    UploadMesh(&mesh, false);
    return mesh;
}
inline BlockAtlasAssets& load_block_atlas() {
    static BlockAtlasAssets assets{};
    static bool attempted = false;
    if (!attempted) {
        attempted = true;
        Image image=make_block_atlas_image();
        if(image.data!=nullptr) {
            assets.texture=LoadTextureFromImage(image);
            const bool validAtlas=image.width==64&&image.height==64;
            UnloadImage(image);
            if(validAtlas&&assets.texture.id!=0) {
                SetTextureFilter(assets.texture, TEXTURE_FILTER_POINT);
                assets.material = LoadMaterialDefault();
                SetMaterialTexture(&assets.material, MATERIAL_MAP_DIFFUSE, assets.texture);
                for(int tile=0;tile<16;++tile) {
                    int side=tile,top=tile,bottom=tile;
                    if(tile==2) { side=2; top=1; bottom=0; }
                    assets.meshes[tile]=make_block_mesh(side,top,bottom);
                }
            } else if (assets.texture.id != 0) {
                UnloadTexture(assets.texture);
                assets.texture = Texture2D{};
            }
        }
    }
    return assets;
}
inline bool draw_textured_block(BlockAtlasAssets& assets, mc::world::BlockId id, Vector3 center) {
    const int tile = block_atlas_tile(id);
    if (assets.texture.id == 0 || assets.meshes[tile].vertexCount == 0) return false;
    assets.material.maps[MATERIAL_MAP_DIFFUSE].color =
        WHITE;
    DrawMesh(assets.meshes[tile], assets.material, MatrixTranslate(center.x, center.y, center.z));
    return true;
}
inline Rectangle block_atlas_rect(mc::world::BlockId id) {
    const int tile = block_atlas_tile(id);
    return Rectangle{float((tile % 4) * 16), float((tile / 4) * 16), 16.0f, 16.0f};
}
inline void unload_block_atlas(BlockAtlasAssets& assets) {
    for (Mesh& mesh : assets.meshes) {
        if (mesh.vertexCount > 0) UnloadMesh(mesh);
    }
    if (assets.texture.id != 0) UnloadTexture(assets.texture);
}
}
