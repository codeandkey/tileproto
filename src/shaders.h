#pragma once

const char* vs_render = "#version 130\n"
			"attribute vec2 position;\n"
			"attribute vec2 in_texcoord;\n"
			"varying vec2 texcoord;\n"
			"uniform mat4x4 transform;\n"
			"void main(void) {\n"
			"	gl_Position = transform * vec4(position, 0.0f, 1.0f);\n"
			"	texcoord = in_texcoord;\n"
			"}\n";

const char* fs_render = "#version 130\n"
			"uniform sampler2D tex;\n"
			"varying vec2 texcoord;\n"
			"void main(void) {\n"
			"	gl_FragColor = texture(tex, texcoord);\n"
			"}\n";
