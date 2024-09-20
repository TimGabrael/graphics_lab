#pragma once
#include "util.h"

struct FontAtlasData {
    struct Glyph {
        uint64_t unicode;
        glm::vec2 start_uv;
        glm::vec2 end_uv;
        glm::vec2 start;
        glm::vec2 size;
        float advance;
    };
    std::vector<Glyph> glyphs;
    glm::vec2 white_uv;
    bool Load(Texture2D& tex, const std::string& font_file, int size, bool subpixel);
};

