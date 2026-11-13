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
        case BlockId::Stone: return 0;
        case BlockId::Grass: return 1;
        case BlockId::Dirt: return 2;
        case BlockId::Sand: return 3;
        case BlockId::Water: return 4;
        case BlockId::Bedrock: return 5;
        case BlockId::Wood: return 6;
        case BlockId::Leaves: return 7;
        case BlockId::Gravel: return 8;
        case BlockId::CoalOre: return 9;
        case BlockId::IronOre: return 10;
        default: return 0;
    }
}
struct BlockAtlasAssets {
    Texture2D texture{};
    Material material{};
    Mesh meshes[11]{};
};
inline BlockAtlasAssets& load_block_atlas() {
    static BlockAtlasAssets assets{};
    static bool attempted = false;
    if (!attempted) {
        attempted = true;
        const auto bytes = decode_block_atlas_base64();
        Image image = LoadImageFromMemory(".png", bytes.data(), static_cast<int>(bytes.size()));
        if (image.data != nullptr) {
            assets.texture = LoadTextureFromImage(image);
            UnloadImage(image);
            SetTextureFilter(assets.texture, TEXTURE_FILTER_POINT);
            assets.material = LoadMaterialDefault();
            SetMaterialTexture(&assets.material, MATERIAL_MAP_DIFFUSE, assets.texture);
            for (int tile = 0; tile < 11; ++tile) {
                Mesh& mesh = assets.meshes[tile];
                mesh = GenMeshCube(2.0f, 2.0f, 2.0f);
                const float u0 = float(tile % 4) * 0.25f;
                const float v0 = float(tile / 4) * 0.25f;
                if (mesh.texcoords != nullptr) {
                    for (int vertex = 0; vertex < mesh.vertexCount; ++vertex) {
                        mesh.texcoords[vertex * 2] = u0 + mesh.texcoords[vertex * 2] * 0.25f;
                        mesh.texcoords[vertex * 2 + 1] = v0 + mesh.texcoords[vertex * 2 + 1] * 0.25f;
                    }
                }
                UploadMesh(&mesh, false);
            }
        }
    }
    return assets;
}
inline bool draw_textured_block(BlockAtlasAssets& assets, mc::world::BlockId id, Vector3 center) {
    const int tile = block_atlas_tile(id);
    if (assets.texture.id == 0 || assets.meshes[tile].vertexCount == 0) return false;
    assets.material.maps[MATERIAL_MAP_DIFFUSE].color =
        id == mc::world::BlockId::Water ? Fade(WHITE, 0.68f) : WHITE;
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
}
