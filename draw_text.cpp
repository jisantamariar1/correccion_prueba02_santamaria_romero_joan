#include "draw_text.h"

#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "arial_ttf.h"

class _free_type_initializer {
public:
    FT_Library ft;
    FT_Face face;

    _free_type_initializer() {
        ft = nullptr;
        face = nullptr;

        FT_Error error = FT_Init_FreeType(&ft);

        if(!error) {
            FT_New_Memory_Face(ft, arial_ttf::data, arial_ttf::data_len, 0, &face);
        }
    }

    ~_free_type_initializer() {
        if(face) FT_Done_Face(face);
        if(ft) FT_Done_FreeType(ft);
    }
};

std::shared_ptr<_free_type_initializer> _freetype = nullptr;
//-----

void init_freetype() {
    _freetype = std::make_shared<_free_type_initializer>();
}

void draw_text_to_texture(unsigned char* texture, int tex_width, int tex_height,
                          const char* text, int x, int y,
                          int font_size) {
    // FT_Library ft;
    // if (FT_Init_FreeType(&ft)) return;

    // FT_Face face;
    // //if (FT_New_Face(ft, font_path, 0, &face)) return;
    // if (FT_New_Memory_Face(ft, arial_ttf::data, arial_ttf::data_len, 0, &face)) return;
    
    //FT_Set_Pixel_Sizes(face, 0, font_size);
    FT_Face face = _freetype->face;

    FT_Set_Pixel_Sizes(face, 0, font_size);

    int pen_x = x;
    for (const char* p = text; *p; p++) {
        if (FT_Load_Char(face, *p, FT_LOAD_RENDER)) continue;

        FT_Bitmap& bmp = face->glyph->bitmap;
        int bmp_w = bmp.width;
        int bmp_h = bmp.rows;

        for (int j = 0; j < bmp_h; j++) {
            for (int i = 0; i < bmp_w; i++) {
                int tx = pen_x + face->glyph->bitmap_left + i;
                int ty = y - face->glyph->bitmap_top + j;

                if (tx < 0 || tx >= tex_width || ty < 0 || ty >= tex_height) continue;

                unsigned char r_fg = 0;
                unsigned char g_fg = 0;
                unsigned char b_fg = 255;

                unsigned char alpha = bmp.buffer[j * bmp.pitch + i];
                
                float a = alpha / 255.0f;
                int idx = (ty * tex_width + tx) * 4;

                // Fondo (lo que ya está en la textura)
                unsigned char r_bg = texture[idx + 0];
                unsigned char g_bg = texture[idx + 1];
                unsigned char b_bg = texture[idx + 2];
                unsigned char a_bg = texture[idx + 3];

                // Mezcla: texto sobre fondo (premultiplicado simple)
                texture[idx + 0] = (unsigned char)(r_fg * a + r_bg * (1.0f - a));
                texture[idx + 1] = (unsigned char)(g_fg * a + g_bg * (1.0f - a));
                texture[idx + 2] = (unsigned char)(b_fg * a + b_bg * (1.0f - a));
                texture[idx + 3] = a_bg>alpha?a_bg:alpha; // o simplemente 255 si usas fondo opaco
            }
        }

        pen_x += face->glyph->advance.x >> 6;
    }

    // FT_Done_Face(face);
    // FT_Done_FreeType(ft);
}