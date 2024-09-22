#include "atlas.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#include FT_COLOR_H
#include <iostream>
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#include <algorithm>

bool FontAtlasData::Load(Texture2D& tex, const std::string& font_file, int size, bool subpixel) {
    FT_Library library;
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        std::cout << "failed to load freetype" << std::endl;
    }
    FT_Face face;
    error = FT_New_Face(library, font_file.c_str(), 0, &face);
    if(error) {
        std::cout << "failed load fontfile" << std::endl;
        FT_Done_FreeType(library);
        return false;
    }
    FT_Library_SetLcdFilter(library, FT_LCD_FILTER_DEFAULT);
    FT_Set_Pixel_Sizes(face, 0, size);
    constexpr size_t unicode_count = 0x1FFFF;
    constexpr int32_t PADDING = 1;
    constexpr float PADDING_F = static_cast<float>(PADDING);

    size_t available_count = 0;
    for(size_t i = 0; i < unicode_count; ++i) {
        FT_UInt char_idx = FT_Get_Char_Index(face, i);
        if(char_idx == 0) {
            continue;
        }
        available_count += 1;
    }
    std::cout << "available_count: " << available_count << std::endl;

    uint32_t val = (uint32_t)((size + PADDING * 2) * sqrt(available_count));
    uint32_t num_nodes = 0x200; // 512 is the smallest possible size
    while(val >= num_nodes) {
        num_nodes <<= 1;
    }
    uint32_t load_flags = subpixel ? FT_LOAD_TARGET_LCD : FT_LOAD_COLOR;

    stbrp_context ctx;
    stbrp_node* nodes = new stbrp_node[num_nodes];
    stbrp_init_target(&ctx, num_nodes, num_nodes, nodes, num_nodes);

    std::vector<stbrp_rect> rects;
    for(size_t i = 0; i < unicode_count; ++i) {
        FT_UInt char_idx = FT_Get_Char_Index(face, i);
        if(char_idx == 0) {
            continue;
        }
        FT_Error err = FT_Load_Char(face, i, load_flags);
        if(err) {
            continue;
        }
        stbrp_rect rect;
        rect.x = 0;
        rect.y = 0;
        rect.w = face->glyph->metrics.width / 64 + PADDING * 2;
        rect.h = face->glyph->metrics.height / 64 + PADDING * 2;
        rect.id = i;
        rect.was_packed = false;
        rects.push_back(rect);
    }

    bool all_packed = stbrp_pack_rects(&ctx, rects.data(), rects.size());
    if(!all_packed) {
        std::cout << "couldn't pack all glyphs in: " << num_nodes << "x" << num_nodes << std::endl;
        delete[] nodes;
        FT_Done_FreeType(library);
        return false;
    }
    delete[] nodes;


    uint32_t* data = new uint32_t[ctx.width * ctx.height];
    memset(data, 0, sizeof(uint32_t) * ctx.width * ctx.height);
    for(size_t i = 0; i < rects.size(); ++i) {
        FT_Error err = FT_Load_Char(face, rects.at(i).id, load_flags | FT_LOAD_RENDER);
        if(err) {
            std::cout << "failed to load glyph: (" << rects.at(i).id << ")" << std::endl;
            continue;
        }
        FontAtlasData::Glyph glyph;
        const FT_Glyph_Metrics& metrics = face->glyph->metrics;
        glyph.size = {rects.at(i).w, rects.at(i).h};
        const int32_t advance = face->glyph->advance.x / 64;
        glyph.advance = (float)advance;

        glyph.start_uv = {(static_cast<float>(rects.at(i).x) + 1.0f + 0.5f) / static_cast<float>(ctx.width), (static_cast<float>(rects.at(i).y - 1.0f - 0.5f)) / static_cast<float>(ctx.height)};
        glyph.end_uv = {(static_cast<float>(rects.at(i).x + rects.at(i).w) + 1.0f + 0.5f) / static_cast<float>(ctx.width), (static_cast<float>(rects.at(i).y + rects.at(i).h) - 1.0f - 0.5f) / static_cast<float>(ctx.height)};

        const int32_t sx = metrics.horiBearingX / 64;
        const int32_t sy = metrics.horiBearingY / 64;
        //glyph.start = {-0.5f + static_cast<float>(sx), (size + 0.5f) - static_cast<float>(sy)};
        glyph.start = {0.5f + static_cast<float>(sx), (size + 0.5f) - static_cast<float>(sy)};
        //glyph.start = {0.5f, (size + 0.5f) - static_cast<float>(sy)};
        glyph.unicode = rects.at(i).id;
        this->glyphs.push_back(glyph);

        uint32_t width = 0;
        uint32_t height = 0;
        FT_Bitmap* bitmap = &face->glyph->bitmap;
        if(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_LCD) {
            width = bitmap->width / 3;
            height = bitmap->rows;
            for(unsigned int y = 0; y < bitmap->rows; ++y) {
                for(unsigned int x = 0; x < bitmap->width; x += 3) {
                    uint8_t r = bitmap->buffer[y * bitmap->pitch + x + 0];
                    uint8_t g = bitmap->buffer[y * bitmap->pitch + x + 1];
                    uint8_t b = bitmap->buffer[y * bitmap->pitch + x + 2];
                    uint8_t a = std::max(std::max(r, g), b);
                    uint32_t col = r | (g << 8) | (b << 16) | (a << 24);
                    uint32_t data_idx = (y + rects.at(i).y + PADDING) * ctx.width + (x / 3 + rects.at(i).x + PADDING);
                    data[data_idx] = col;
                }
            }
        }
        else if(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
            FT_Glyph_Format fmt;
            width = bitmap->width;
            height = bitmap->rows;
            for(unsigned int y = 0; y < bitmap->rows; ++y) {
                for(unsigned int x = 0; x < bitmap->width; x += 1) {
                    uint8_t b = bitmap->buffer[y * bitmap->pitch + 4 * x + 0];
                    uint8_t g = bitmap->buffer[y * bitmap->pitch + 4 * x + 1];
                    uint8_t r = bitmap->buffer[y * bitmap->pitch + 4 * x + 2];
                    uint8_t a = bitmap->buffer[y * bitmap->pitch + 4 * x + 3];
                    uint32_t col = r | (g << 8) | (b << 16) | (a << 24);
                    uint32_t data_idx = (y + rects.at(i).y + PADDING) * ctx.width + (x + rects.at(i).x + PADDING); 
                    data[data_idx] = col;
                }
            }
        }
        else if(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
            FT_Glyph_Format fmt;
            width = bitmap->width;
            height = bitmap->rows;
            for(unsigned int y = 0; y < bitmap->rows; ++y) {
                for(unsigned int x = 0; x < bitmap->width; x += 1) {
                    uint8_t a = bitmap->buffer[y * bitmap->pitch + x + 0];
                    uint8_t r = 0xFF;
                    uint8_t g = 0xFF;
                    uint8_t b = 0xFF;
                    uint32_t col = r | (g << 8) | (b << 16) | (a << 24);
                    uint32_t data_idx = (y + rects.at(i).y + PADDING) * ctx.width + (x + rects.at(i).x + PADDING);
                    data[data_idx] = col;
                }
            }
        }
    }


    FT_Done_Face(face);
    FT_Done_FreeType(library);

    tex = CreateTexture2D(data, ctx.width, ctx.height);
    delete[] data;
    
    return true;
}









