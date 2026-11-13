#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../world/Block.hpp"
#include <string>
#include <vector>

namespace mc::render {
inline std::vector<unsigned char> decode_block_atlas_base64() {
    static constexpr char encoded[] = R"ATLAS(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)ATLAS";
    auto value = [](unsigned char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    std::vector<unsigned char> out;
    int bits = 0, accumulator = 0;
    for (unsigned char c : encoded) {
        if (c == '=') break;
        const int v = value(c);
        if (v < 0) continue;
        accumulator = (accumulator << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<unsigned char>((accumulator >> bits) & 0xff));
        }
    }
    return out;
}
inline int block_atlas_tile(mc::world::BlockId id) {
    using mc::world::BlockId;
    switch (id) {
        case BlockId::Stone: return 5;
        case BlockId::Grass: return 1;
        case BlockId::Dirt: return 0;
        case BlockId::Sand: return 6;
        case BlockId::Water: return 7;
        case BlockId::Bedrock: return 8;
        case BlockId::Wood: return 3;
        case BlockId::Leaves: return 2;
        case BlockId::Gravel: return 9;
        case BlockId::CoalOre: return 10;
        case BlockId::IronOre: return 11;
        case BlockId::Planks: return 12;
        case BlockId::Stick: return 13;
        case BlockId::CraftingTable: return 14;
        default: return 0;
    }
}
struct BlockAtlasAssets {
    Texture2D texture{};
    Material material{};
    Mesh meshes[15]{};
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
        const auto bytes = decode_block_atlas_base64();
        Image image = LoadImageFromMemory(".png", bytes.data(), static_cast<int>(bytes.size()));
        if (image.data != nullptr) {
            assets.texture = LoadTextureFromImage(image);
            const bool validAtlas = image.width == 64 && image.height == 64;
            UnloadImage(image);
            if (validAtlas && assets.texture.id != 0) {
                SetTextureFilter(assets.texture, TEXTURE_FILTER_POINT);
                assets.material = LoadMaterialDefault();
                SetMaterialTexture(&assets.material, MATERIAL_MAP_DIFFUSE, assets.texture);
                for (int tile = 0; tile < 15; ++tile) {
                    const int topTile = tile;
                    const int bottomTile = (tile == 1) ? 0 : tile; // Grass bottom = dirt.
                    assets.meshes[tile] = make_block_mesh(tile, topTile, bottomTile);
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
