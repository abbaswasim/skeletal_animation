// Wasim Abbas
// http://www.waZim.com
// Copyright (c) 2019
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the 'Software'),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Version: 1.0.0

#include <iostream>
#include <map>

#include "geometry.hpp"
#include "math/rormatrix4_functions.hpp"

#define SCR_Width 1024
#define SCR_Height 768

float     aspect_ratio = 1.0f;
float     box_color[4] = {0.0f, 1.0f, 0.0f, 0.5f};
Geometry *cube         = nullptr;

static const char *vertex_shader_src =
	"#version 330\n"
	"in vec4 position;\n"
	"uniform mat4 model_view_projection;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = model_view_projection * position;\n"
	"}\n";

static const char *fragment_shader_src =
	"#version 330\n"
	"out vec4 fragment;\n"
	"uniform vec4 color;\n"
	"void main()\n"
	"{\n"
	"    fragment = color;\n"
	"}\n";

void idle()
{}

void setup()
{
	cube = create_cube(3.0f, vertex_shader_src, fragment_shader_src);
}

void animate()
{}

ror::Matrix4f get_mvp()
{
	auto translation = ror::matrix4_translation(0.0f, 0.0f, -10.0f);
	auto rotation_y  = ror::matrix4_rotation_around_y(ror::to_radians((float) glfwGetTime() * 70));
	auto projection  = ror::make_perspective(ror::to_radians(60.0f), aspect_ratio, 0.5f, 100.0f);

	auto model = translation * rotation_y;

	return projection * model;
}

void display()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	auto mvp = get_mvp();

	cube->draw(mvp.m_values, GL_LINES, box_color);
}

void key(GLFWwindow *window, int k, int s, int action, int mods)
{
	(void) s;
	(void) mods;

	switch (k)
	{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		case GLFW_KEY_SPACE:
		case GLFW_KEY_G:
			break;
		case GLFW_KEY_W:
			break;
		case GLFW_KEY_S:
			break;
		case GLFW_KEY_C:
			break;
		case GLFW_KEY_R:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case GLFW_KEY_F:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case GLFW_KEY_I:
			break;
		case GLFW_KEY_O:
			break;
		default:
			return;
	}
}

void resize(GLFWwindow *window, int width, int height)
{
	if (width == 0 || height == 0)
		return;

	int width_, height_;
	glfwGetFramebufferSize(window, &width_, &height_);
	aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);
}

int main(int argc, char **argv)
{
	GLFWwindow *window;

	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_DEPTH_BITS, 16);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);

#if defined __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#elif defined __linux__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#endif

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

	window = glfwCreateWindow(SCR_Width, SCR_Height, "Simple Skeletal Animation", NULL, NULL);

	if (!window)
	{
		fprintf(stderr, "Failed to open GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	glfwSetKeyCallback(window, key);
	glfwSetWindowSizeCallback(window, resize);

	setup();

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		animate();
		display();

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Terminate GLFW
	glfwTerminate();
}
