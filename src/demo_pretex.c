#include "demo_pretex.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <GLXW/glxw.h>
#include <GL/freeglut.h>

#include "stb_image.h"
#include "tileproto.h"
#include "linmath.h"

#define BLOCKS 4
#define CHUNKSIZE 32
#define BLOCKPIXELS 16

#define HACCEL 0.08f
#define HMAX 0.8f
#define VACCEL 0.08f
#define VMAX 0.8f
#define DECAY 1.2f

static const char* blocktex[BLOCKS] = {
	NULL,
	"res/grass.png",
	"res/stone.png",
	"res/brick.png"
};

typedef struct _live_chunk {
	int cx, cy;
	unsigned tex, fbo;
	struct _live_chunk* next, *prev;
} live_chunk;

static unsigned pretex_init = 0;
static unsigned pretex_texlist[BLOCKS] = {0};
static unsigned chunk_vbo, chunk_vao, block_vbo, block_vao;
static live_chunk* chunk_list, *chunk_list_tail;
static float camerax, cameray;
static float cxspeed, cyspeed;

void demo_pretex_query_wdata(int cx, int cy, uint8_t* data); /* cx, cy: chunk numbers */
live_chunk* demo_pretex_compile_chunk(int cx, int cy);
void demo_pretex_render_chunk(live_chunk* c);
void demo_pretex_free_chunk(live_chunk* c);

void demo_pretex_request_chunk(int cx, int cy);
int demo_pretex_chunk_loaded(int cx, int cy);

int demo_pretex_render(void) {
	if (!pretex_init) {
		int r = demo_pretex_init();
		if (r) return r;
	}

	/*
	 * the pretex demo is just demonstrating an interesting method for rendering tile worlds
	 * the demo also benchmarks how quickly a world can be navigated and traversed using this technique
	 */

	/*
	 * render process:
	 *  1) iterate through current live chunks, cull dead, render live
	 *  2) determine if we need to load up any newer chunks
	 *  (3) optional render all new chunks
	 */

	//test_chunk = demo_pretex_compile_chunk(0, 0);

	if (glfwGetKey(wh, GLFW_KEY_RIGHT)) {
		cxspeed += HACCEL;
	}

	if (glfwGetKey(wh, GLFW_KEY_LEFT)) {
		cxspeed -= HACCEL;
	}

	if (glfwGetKey(wh, GLFW_KEY_UP)) {
		cyspeed += VACCEL;
	}

	if (glfwGetKey(wh, GLFW_KEY_DOWN)) {
		cyspeed -= VACCEL;
	}

	if (fabs(cxspeed) > HMAX) cxspeed /= (fabs(cxspeed)/HMAX);
	if (fabs(cyspeed) > VMAX) cyspeed /= (fabs(cyspeed)/VMAX);

	camerax += cxspeed;
	cameray += cyspeed;

	cxspeed /= DECAY;
	cyspeed /= DECAY;

	mat4x4_translate(view, -camerax, -cameray, 0.0f);

	for (int cx = camerax/CHUNKSIZE - 1; cx * CHUNKSIZE <= camerax + CAMERASIZE*RATIO; ++cx) {
		for (int cy = cameray / CHUNKSIZE - 1; cy * CHUNKSIZE <= cameray + CAMERASIZE; ++cy) {
			demo_pretex_request_chunk(cx, cy);
		}
	}

	live_chunk* c = chunk_list, *tmp;

	while (c) {
		if (c->cx * CHUNKSIZE >= camerax + CAMERASIZE*RATIO || (c->cx+1) * CHUNKSIZE <= camerax || (c->cy+1)*CHUNKSIZE <= cameray || c->cy*CHUNKSIZE >= cameray+CAMERASIZE) {
			tmp = c->next;
			demo_pretex_free_chunk(c);
			c = tmp;
			continue;
		}

		demo_pretex_render_chunk(c);
		c = c->next;
	}

	return 0;
}

int demo_pretex_init(void) {
	pretex_init = 1;
	printf("demo_pretex: initializing\n");
	printf("demo_pretex: chunk size = %dx%d blocks\n", CHUNKSIZE, CHUNKSIZE);
	printf("demo_pretex: selecting chunk data from %d distinct blocktypes\n", BLOCKS);
	printf("demo_pretex: loading block textures");

	glGenTextures(BLOCKS - 1, pretex_texlist + 1);

	for (int i = 1; i < BLOCKS; ++i) {
		int w, h, rw;
		unsigned char* stbd = stbi_load(blocktex[i], &w, &h, NULL, 4);
		if (!stbd) {
			printf("\ntex fail: %s\n", blocktex[i]);
			return 1;
		}
		unsigned char* next = malloc(w*h*4);
		rw = 4*w;
		for (int j = 0; j < h; ++j) {
			memcpy(next + j * rw, stbd + (h-1) * rw - j*rw, rw);
		}
		glBindTexture(GL_TEXTURE_2D, pretex_texlist[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, next);
		free(next);
		stbi_image_free(stbd);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		printf(".");
	}
	printf(" done\n");

	printf("demo_pretex: initializing vertex arrays\n");
	float verts[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		CHUNKSIZE, 0.0f, 1.0f, 0.0f,
		0.0f, CHUNKSIZE, 0.0f, 1.0f,

		0.0f, CHUNKSIZE, 0.0f, 1.0f,
		CHUNKSIZE, 0.0f, 1.0f, 0.0f,
		CHUNKSIZE, CHUNKSIZE, 1.0f, 1.0f
	};

	float blockverts[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 1.0f,

		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};

	glGenVertexArrays(1, &block_vao);
	glBindVertexArray(block_vao);
	glGenBuffers(1, &block_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, block_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, blockverts, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, NULL);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*) (sizeof(float)*2));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenVertexArrays(1, &chunk_vao);
	glBindVertexArray(chunk_vao);
	glGenBuffers(1, &chunk_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, chunk_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, verts, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, NULL);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*) (sizeof(float)*2));

	glEnableVertexAttribArray(0); /* all VAOs use this so we don't really need to worry about the state too much */
	glEnableVertexAttribArray(1);
	return 0;
}

void demo_pretex_free(void) {
	if (!pretex_init) return;
	printf("demo_pretex: cleaning up\n");

	glDeleteBuffers(1, &block_vbo);
	glDeleteVertexArrays(1, &block_vao);
	glDeleteBuffers(1, &chunk_vbo);
	glDeleteVertexArrays(1, &chunk_vao);

	glDeleteTextures(BLOCKS - 1, pretex_texlist + 1);
}

void demo_pretex_query_wdata(int cx, int cy, uint8_t* dest) {
	/*
	 * normally this would pull world information from the disk.
	 * however, for the purposes of this demo random data will suffice.
	 */

	for (int i = 0; i < CHUNKSIZE*CHUNKSIZE; ++i) {
		dest[i] = rand() % BLOCKS;
	}
}

void demo_pretex_render_chunk(live_chunk* c) {
	/* this is fortunately rather straightforward.
	 * we translate the chunk VBO over and render with the live chunk texture */

	mat4x4_translate(model, c->cx * CHUNKSIZE, c->cy * CHUNKSIZE, 0.0f);
	update_mats();
	glBindVertexArray(chunk_vao);
	glBindTexture(GL_TEXTURE_2D, c->tex);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

live_chunk* demo_pretex_compile_chunk(int cx, int cy) {
	live_chunk* output = malloc(sizeof *output);

	output->cx = cx;
	output->cy = cy;
	output->next = output->prev = NULL;
	glGenTextures(1, &output->tex);
	glBindTexture(GL_TEXTURE_2D, output->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHUNKSIZE * BLOCKPIXELS, CHUNKSIZE * BLOCKPIXELS, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &output->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, output->fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, output->tex, 0);

	GLenum db[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, db);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("demo_pretex: FBO init failed\n");
		return NULL;
	}

	/*
	 * this is the most critical function in the pretex demo
	 * taking known block data and compiling it into a chunk texture which
	 * can be quickly used later in the render process instead of rendering
	 * blocks on their own
	 *
	 * this means that we must render blocks on their own here to an offscreen texture.
	 * so, we have to set up an FBO and prepare to render to it
	 * we may also have to perform hblock reduction and appropriate VBO generation here
	 * as a result this is the primary source of overhead in the technique
	 */

	uint8_t blockdata[CHUNKSIZE * CHUNKSIZE];
	demo_pretex_query_wdata(0, 0, blockdata);

	/* generating hblocks and VBOs might be so costly that it will be better to just render each block individually */

	glBindFramebuffer(GL_FRAMEBUFFER, output->fbo);
	glBindVertexArray(block_vao);
	glViewport(0, 0, CHUNKSIZE * BLOCKPIXELS, CHUNKSIZE * BLOCKPIXELS);

	mat4x4 xform, final;
	mat4x4_ortho(xform, 0.0f, CHUNKSIZE, 0.0f, CHUNKSIZE, -0.1f, 0.1f);

	for (int y = 0; y < CHUNKSIZE; ++y) {
		for (int x = 0; x < CHUNKSIZE; ++x) {
			/* render the block located at (x, y) relative to the chunk origin into the texture */
			mat4x4_translate(model, x, y, 0);
			mat4x4_mul(final, xform, model);
			glUniformMatrix4fv(loc_xform, 1, GL_FALSE, (float*) *final);

			glBindTexture(GL_TEXTURE_2D, pretex_texlist[blockdata[x + y * CHUNKSIZE]]);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);

	return output;
}

void demo_pretex_free_chunk(live_chunk* c) {
	glDeleteTextures(1, &c->tex);
	glDeleteFramebuffers(1, &c->fbo);

	if (c->prev) {
		c->prev->next = c->next;
	} else {
		chunk_list = c->next;
	}

	if (c->next) {
		c->next->prev = c->prev;
	} else {
		chunk_list_tail = c->prev;
	}

	free(c);
}

int demo_pretex_chunk_loaded(int cx, int cy) {
	live_chunk* c = chunk_list;
	while (c) {
		if (c->cx == cx && c->cy == cy) return 1;
		c = c->next;
	}
	return 0;
}

void demo_pretex_request_chunk(int cx, int cy) {
	if (demo_pretex_chunk_loaded(cx, cy)) return;
	live_chunk* c = demo_pretex_compile_chunk(cx, cy);
	if (chunk_list_tail) {
		c->prev = chunk_list_tail;
		chunk_list_tail->next = c;
	} else {
		chunk_list = c;
	}
	chunk_list_tail = c;
}
