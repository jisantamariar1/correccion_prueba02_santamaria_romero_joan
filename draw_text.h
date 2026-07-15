#ifndef DRAW_TEXT_H
#define DRAW_TEXT_H

void init_freetype();

void draw_text_to_texture(unsigned char* texture, int tex_width, int tex_height,
                          const char* text, int x, int y,
                          int font_size);

#endif // DRAW_TEXT_H