#pragma once
#include "raylib.h"
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
inline Texture2D load_block_atlas() {
    static Texture2D texture{};
    static bool attempted = false;
    if (!attempted) {
        attempted = true;
        const auto bytes = decode_block_atlas_base64();
        Image image = LoadImageFromMemory(".png", bytes.data(), static_cast<int>(bytes.size()));
        if (image.data != nullptr) {
            texture = LoadTextureFromImage(image);
            UnloadImage(image);
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
        }
    }
    return texture;
}
inline Rectangle block_atlas_rect(mc::world::BlockId id) {
    int tile = 0;
    using mc::world::BlockId;
    switch (id) {
        case BlockId::Stone: tile = 0; break;
        case BlockId::Grass: tile = 1; break;
        case BlockId::Dirt: tile = 2; break;
        case BlockId::Sand: tile = 3; break;
        case BlockId::Water: tile = 4; break;
        case BlockId::Bedrock: tile = 5; break;
        case BlockId::Wood: tile = 6; break;
        case BlockId::Leaves: tile = 7; break;
        case BlockId::Gravel: tile = 8; break;
        case BlockId::CoalOre: tile = 9; break;
        case BlockId::IronOre: tile = 10; break;
        default: tile = 0; break;
    }
    return Rectangle{float((tile % 4) * 16), float((tile / 4) * 16), 16.0f, 16.0f};
}
}
