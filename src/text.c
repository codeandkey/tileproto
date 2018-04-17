#include "text.h"

#include <GLXW/glxw.h>

static unsigned init = 0;
static FT_Library ctx;
static unsigned vs, fs, prg, loc_tex, loc_col;

void tk_text_init(void);
float pixel_map(int p, int view);
static unsigned int make_shader(const char* str, GLenum type);

const char* tk_text_vs = "#version 130\nattribute vec2 p;\nattribute vec2 ti;\nvarying vec2 t;\n"
			 "void main() { gl_Position = vec4(p, 0, 1); t = ti; }\n";
const char* tk_text_fs = "#version 130\nuniform sampler2D tx;\nuniform vec4 c;\nvarying vec2 t;\n"
			 "void main() { gl_FragColor = vec4(1.0, 1.0, 1.0, texture2D(tx, t).r)*c; }\n";

tk_font* tk_font_init(const char* filename, int size) {
	if (!init) tk_text_init();
	if (!init) return NULL;

	tk_font* output = malloc(sizeof* output);
	memset(output, 0, sizeof *output);

	output->col[0] = output->col[1] = output->col[2] = output->col[3] = 1.0f;

	if (!output) return NULL;

	int er = FT_New_Face(ctx, filename, 0, &output->face);

	if (er == FT_Err_Unknown_File_Format) {
		tk_die("Invalid font format: %s\n", filename);
	} else if (er) {
		tk_die("Failed loading: %s\n", filename);
	}

	output->size_px = size;
	FT_Set_Pixel_Sizes(output->face, 0, size);
	glGenTextures(TK_TEXT_GLYPHS, output->glyphs);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (int i = 0; i < TK_TEXT_GLYPHS; ++i) {
		glBindTexture(GL_TEXTURE_2D, output->glyphs[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		if (FT_Load_Char(output->face, i, FT_LOAD_RENDER)) {
			continue;
		}

		FT_GlyphSlot g = output->face->glyph;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, g->bitmap.width, g->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	tk_log("loaded glyphs for %s\n", filename);

	return output;
}

void tk_font_free(tk_font* dest) {
	free(dest);
}

void tk_font_render(tk_font* p, int x, int y, int flags, const char* fmt, ...) {
	if (!init) return;

	va_list args;
	va_start(args, fmt);

	char str[TK_TEXT_MAXLEN] = {0};
	int len;
	vsnprintf(str, sizeof str, fmt, args);
	va_end(args);

	len = strlen(str);

	glUseProgram(prg);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	glUniform4fv(loc_col, 1, p->col);

	/* keep viewport data */
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	float cx, cy; /* current drawing origin */
	cx = pixel_map(x, viewport[2]);
	cy = pixel_map(y, viewport[3]);

	/* compute total width in worldspace */
	float total_width = 0.0f;
	float sx = 2.0f / (float) viewport[2], sy = 2.0f / (float) viewport[3];

	/* (sx, sy) is the screenspace size of 1 pixel */
	for (int i = 0; i < len; ++i) {
		if (FT_Load_Char(p->face, str[i], FT_LOAD_DEFAULT)) {
			tk_log("Glpyh load failed for [%02x %c]\n", str[i], str[i]);
			continue;
		}

		FT_GlyphSlot g = p->face->glyph;
		total_width += (g->advance.x >> 6) * sx;
	}

	float xoff = 0.0f;

	if (flags & TK_TEXT_CENTER) {
		xoff -= total_width / 2.0f;
	} else if (flags & TK_TEXT_RIGHT) {
		xoff -= total_width;
	}

	for (int i = 0; i < len; ++i) {
		if (FT_Load_Char(p->face, str[i], FT_LOAD_RENDER)) {
			tk_log("Glpyh load failed for [%02x %c]\n", str[i], str[i]);
			continue;
		}

		FT_GlyphSlot g = p->face->glyph;

		float gw = g->bitmap.width * sx, gh = g->bitmap.rows * sy;
		float xl = cx + g->bitmap_left * sx, yt = cy + g->bitmap_top * sy;

		float verts[16] = {
			xl + xoff, yt, 0.0f, 0.0f,
			xl + xoff + gw, yt, 1.0f, 0.0f,
			xl + xoff, yt - gh, 0.0f, 1.0f,
			xl + xoff + gw, yt - gh , 1.0f, 1.0f,
		};

		unsigned buf, vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &buf);
		glBindBuffer(GL_ARRAY_BUFFER, buf);
		glBufferData(GL_ARRAY_BUFFER, sizeof verts, verts, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, NULL);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*) (sizeof(float)*2));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glBindTexture(GL_TEXTURE_2D, p->glyphs[(int) str[i]]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDeleteBuffers(1, &buf);
		glDeleteVertexArrays(1, &buf);

		cx += (g->advance.x >> 6) * sx;
		cy -= (g->advance.y >> 6) * sy;
	}
}

void tk_text_free(void) {
	if (!init) return;
	init = 0;

	FT_Done_FreeType(ctx);
}

void tk_text_init(void) {
	if (init) return;

	if (FT_Init_FreeType(&ctx)) {
		tk_die("Failed to initialize FT2.\n");
	}

	vs = make_shader(tk_text_vs, GL_VERTEX_SHADER);
	fs = make_shader(tk_text_fs, GL_FRAGMENT_SHADER);

	if (!vs || !fs) {
		tk_die("Shader init fail.\n");
	}

	prg = glCreateProgram();
	glAttachShader(prg, vs);
	glAttachShader(prg, fs);
	glLinkProgram(prg);

	int st;
	glGetProgramiv(prg, GL_LINK_STATUS, &st);
	if (!st) {
		tk_die("Program link fail.\n");
	}

	glUseProgram(prg);
	loc_tex = glGetUniformLocation(prg, "tx");
	glUniform1i(loc_tex, 0);
	loc_col = glGetUniformLocation(prg, "c");
	glUniform4f(loc_col, 1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_BLEND);

	init = 1;
}

float pixel_map(int p, int view) {
	return ((float) p / (float) view) * 2.0f - 1.0f;
}

unsigned int make_shader(const char* source, GLenum type) {
	unsigned int out = glCreateShader(type);
	int len = strlen(source);

	glShaderSource(out, 1, &source, &len);
	glCompileShader(out);
	glGetShaderiv(out, GL_COMPILE_STATUS, &len);

	if (!len) {
		char log[1024] = {0};
		glGetShaderInfoLog(out, 1023, NULL, log);
		printf("shader compile fail %p: %s\n", source, log);
		return 0;
	}

	return out;
}

void tk_font_set_col(tk_font* p, float r, float g, float b, float a) {
	p->col[0] = r;
	p->col[1] = g;
	p->col[2] = b;
	p->col[3] = a;
}
