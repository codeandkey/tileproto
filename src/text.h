#pragma once

#define tk_die(x, ...) { printf("[tk] " x, ##__VA_ARGS__); exit(1); }
#define tk_log(x, ...) { printf("[tk] " x, ##__VA_ARGS__); }

#include <ft2build.h>
#include FT_FREETYPE_H

#define TK_TEXT_GLYPHS 256
#define TK_TEXT_CENTER 1
#define TK_TEXT_RIGHT (1 << 1)
#define TK_TEXT_MAXLEN 128

#include <stdarg.h>

typedef struct _tk_font {
	FT_Face face;
	int size_px;
	float col[4];
	unsigned int glyphs[TK_TEXT_GLYPHS];
} tk_font;

tk_font* tk_font_init(const char* filename, int size);
void tk_font_free(tk_font* dest);

/* x, y treated as pixel-space vector from the lower-left origin to the lower-left corner of the first glpyh */
void tk_font_render(tk_font* p, int x, int y, int flags, const char* fmt, ...);
void tk_font_set_col(tk_font* p, float r, float g, float b, float a);

void tk_text_free(void);
