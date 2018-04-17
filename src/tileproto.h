#pragma once
#include "linmath.h"

#include <GLFW/glfw3.h>

extern GLFWwindow* wh;

/* just some quick vars to allow program modules to easily work with the demo */

extern float camera[4]; /* x, y, width, height */
extern mat4x4 model, view, proj;
extern unsigned loc_xform, prg;

void update_mats(void);

#define WIDTH 1366
#define HEIGHT 768
#define RATIO ((float) WIDTH / (float) HEIGHT)
#define CAMERASIZE 15.0f
