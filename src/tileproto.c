/*
 * tileproto
 *
 * this is basically a quick and dirty proof-of-concept demonstrating some interesting techniques for rendering 2D tile-based worlds.
 * rendering methods make as much usage of multithreading capabilities as possible
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <GLXW/glxw.h>
#include <GLFW/glfw3.h>
#include <GL/glut.h>

#include "shaders.h"
#include "linmath.h"

#include "demo_pretex.h"
#include "tileproto.h"

#define FS 1

GLFWwindow* wh;
unsigned int prg, vs, fs, loc_xform, loc_tex;

float camera[4] = {0.0f, 0.0f, CAMERASIZE*RATIO, CAMERASIZE}; /* adjust width for ratio */
mat4x4 model, view, proj;

unsigned int make_shader(const char* source, GLenum type);
void update_mats(void);

int main(int argc, char** argv) {
	if (!glfwInit()) return 1;

	srand(time(NULL));

	glfwWindowHint(GLFW_SAMPLES, 2); /* 2x msaa */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	wh = glfwCreateWindow(WIDTH, HEIGHT, "tileproto", FS ? glfwGetPrimaryMonitor() : NULL, NULL);
	if (!wh) return 2;

	glfwMakeContextCurrent(wh);
	if (glxwInit()) return 3;

	glViewport(0, 0, WIDTH, HEIGHT);

	/* this won't require any special shaders, set up a quick passthrough */
	vs = make_shader(vs_render, GL_VERTEX_SHADER);
	if (!vs) return 4;

	fs = make_shader(fs_render, GL_FRAGMENT_SHADER);
	if (!fs) return 5;

	prg = glCreateProgram();
	glAttachShader(prg, vs);
	glAttachShader(prg, fs);
	glLinkProgram(prg);

	int status;
	glGetProgramiv(prg, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1024];
		glGetProgramInfoLog(prg, 1023, NULL, log);
		printf("prg fail: %s\n", log);
		return 6;
	}

	glUseProgram(prg);

	loc_xform = glGetUniformLocation(prg, "transform");
	loc_tex = glGetUniformLocation(prg, "tex");

	glUniform1i(loc_tex, 0); /* prep texture unit */
	glActiveTexture(GL_TEXTURE0);

	mat4x4_identity(model);
	mat4x4_identity(view);
	update_mats();

	/* shaders prepped, start up the mainloop */
	while (!glfwWindowShouldClose(wh)) {
		glfwPollEvents();
		if (glfwGetKey(wh, GLFW_KEY_ESCAPE)) break;
		glClear(GL_COLOR_BUFFER_BIT);

		if (demo_pretex_render()) break;
		glfwSwapBuffers(wh);
	}

	demo_pretex_free();
	glfwTerminate();
	return 0;
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

void update_mats(void) {
	/* recompute ortho+view camera matrices */
	mat4x4 viewproj;
	mat4x4_ortho(proj, camera[0], camera[0] + camera[2], camera[1], camera[1] + camera[3], -0.1f, 0.1f);
	mat4x4_mul(viewproj, proj, view);

	mat4x4 final; /* ignoring the viewmat for now */
	mat4x4_mul(final, viewproj, model);

	glUniformMatrix4fv(loc_xform, 1, GL_FALSE, (float*) *final);
}
