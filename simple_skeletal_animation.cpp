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
#include <utility>
#include <vector>

#include "geometry.hpp"
#include "math/rormatrix4.hpp"
#include "math/rormatrix4_functions.hpp"
#include "math/rorvector3.hpp"
#include "skeletal_animation.hpp"

#define SCR_Width 1024
#define SCR_Height 768

bool show_skin     = true;
bool show_skeleton = true;
bool show_cube     = false;

bool do_animate = true;

float             aspect_ratio       = 1.0f;
Geometry *        cube               = nullptr;
Geometry *        astro_boy_skeleton = nullptr;
AnimatedGeometry *astro_boy_skin     = nullptr;
double            old_time           = 0.0;

static const char *vertex_shader_src =
	"#version 330 core\n"
	"layout (location = 0) in vec4 position;\n"
	"layout (location = 1) in vec4 color;\n"
	"uniform mat4 model_view_projection;\n"
	"out vec3 color_out;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = model_view_projection * position;\n"
	"    color_out = color.xyz; \n"
	"}\n";

static const char *fragment_shader_src =
	"#version 330\n"
	"out vec4 fragment;\n"
	"in vec3 color_out;\n"
	"void main()\n"
	"{\n"
	"    fragment = vec4(color_out, 1.0);\n"
	"}\n";

// https://learnopengl.com/Lighting/Basic-Lighting\n
static const char *vertex_shader_lit_src =
	"#version 330 core\n"
	"layout (location = 0) in vec4 position;\n"
	"layout (location = 1) in vec3 normal;\n"
	"layout (location = 2) in vec2 uv;\n"
	"layout (location = 3) in vec4 weights;\n"
	"layout (location = 4) in uvec4 joints;\n"
	"out vec3 position_out;\n"
	"out vec3 normal_out;\n"
	"out vec2 uv_out;\n"
	"uniform mat4 model;\n"
	"uniform mat4 view;\n"
	"uniform mat4 projection;\n"
	"const int joints_max = 44;\n"
	"layout (std140) uniform joint_matrices\n"
	"{\n"
	"    mat4 joints_matrix[joints_max];\n"
	"};\n"
	"void main()\n"
	"{\n"
	"	 mat4 keyframe_transform =\n"
	"		joints_matrix[joints.x] * weights.x +\n"
	"		joints_matrix[joints.y] * weights.y +\n"
	"		joints_matrix[joints.z] * weights.z;\n"
	"	 mat4 model_animated = model * keyframe_transform;\n"
	"    position_out = vec3(model_animated * position);\n"
	"    normal_out = mat3(transpose(inverse(model))) * normal;  \n"
	"    uv_out = uv;  \n"
	"    uv_out.y = 1.0 - uv.y;  \n"
	"    gl_Position = projection * view * vec4(position_out, 1.0);\n"
	"}\n";

static const char *fragment_shader_lit_src =
	"#version 330 core\n"
	"out vec4 fragment;\n"
	"\n"
	"in vec3 position_out;  \n"
	"in vec3 normal_out;  \n"
	"in vec2 uv_out;  \n"
	"uniform sampler2D diffuse_texture;\n"
	"  \n"
	"void main()\n"
	"{\n"
	"	vec3 light_position = vec3(50.0, 20.0, 10.0); \n"
	"	vec3 view_position = vec3(1.0, 1.0, 1.0); \n"
	"	vec3 light_color = vec3(0.9, 0.9, 1.0);\n"
	"	vec3 object_color = vec3(1.0, 1.0, 1.0);\n"
	"    // ambient\n"
	"    float ambientStrength = 0.1;\n"
	"    vec3 ambient = ambientStrength * light_color;\n"
	"		\n"
	"    // diffuse \n"
	"    vec3 norm = normalize(normal_out);\n"
	"    vec3 lightDir = normalize(light_position - position_out);\n"
	"    float diff = max(dot(norm, lightDir), 0.0);\n"
	"    vec3 diffuse = diff * light_color;\n"
	"        \n"
	"    // specular\n"
	"    float specularStrength = 0.5;\n"
	"    vec3 viewDir = normalize(view_position - position_out);\n"
	"    vec3 reflectDir = reflect(-lightDir, norm);  \n"
	"    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
	"    vec3 specular = specularStrength * spec * light_color;  \n"
	"    vec3 result = (ambient + diffuse + specular) * object_color;\n"
	"    fragment = vec4(result, 1.0) * texture(diffuse_texture, uv_out);\n"
	"} \n";

void idle()
{}

void setup()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.14f, 0.14f, 0.14f, 1.0f);

	cube = create_cube(3.5f, ror::Vector3f(0.0f, 0.0f, 3.5f), vertex_shader_src, fragment_shader_src);

	// setup skeleton and get world matrices
	auto astro_boy_matrices = get_world_matrices_for_skeleton(astro_boy_tree, astro_boy_nodes_count);
	astro_boy_skeleton      = get_lines_from_skeleton(astro_boy_matrices, vertex_shader_src, fragment_shader_src);

	std::vector<ror::Matrix4f> astro_boy_joint_matrices;

	for (auto &elem : astro_boy_matrices)
	{
		if (astro_boy_tree[elem.first].m_type == 1)
		{
			astro_boy_joint_matrices.push_back(elem.second.second * get_ror_matrix4(astro_boy_tree[elem.first].m_inverse));
		}
	}

	astro_boy_skin = new AnimatedGeometry(vertex_shader_lit_src, fragment_shader_lit_src, "astro_boy.jpg",
										  sizeof(float) * astro_boy_positions_array_count, astro_boy_positions,
										  sizeof(float) * astro_boy_normals_array_count, astro_boy_normals,
										  sizeof(float) * astro_boy_uvs_array_count, astro_boy_uvs,
										  sizeof(float) * astro_boy_weights_array_count, astro_boy_weights,
										  sizeof(int) * astro_boy_joints_array_count, astro_boy_joints,
										  sizeof(float) * astro_boy_indices_array_count, astro_boy_indices,
										  astro_boy_indices_array_count);

	astro_boy_skin->update_matrices(astro_boy_joint_matrices);
}

std::pair<unsigned int, double> get_keyframe_time()
{
	double new_time = 0.0;

	if (do_animate)
		new_time = glfwGetTime();

	auto delta = new_time - old_time;

	// Note this is very specific to AstroBoy
	static double accumulate_time  = 0.0;
	static int    current_keyframe = 0;
	const double  pf               = 1.166670 / 36.0;

	accumulate_time += delta;
	if (do_animate)
		current_keyframe = accumulate_time / pf;

	if (accumulate_time > 1.66670 || (current_keyframe > astro_boy_animation_keyframes_count - 5))        // Last 5 frames don't quite work with the animation loop, so ignored
	{
		accumulate_time  = 0.0;
		current_keyframe = 0;
	}

	old_time = new_time;

	return std::make_pair(current_keyframe, delta);
}

void animate()
{
	std::vector<ror::Matrix4f> astro_boy_joint_matrices;
	astro_boy_joint_matrices.reserve(astro_boy_nodes_count);

	auto [current_keyframe, delta_time] = get_keyframe_time();

	auto astro_boy_matrices = get_world_matrices_for_skinning(astro_boy_tree, astro_boy_nodes_count, current_keyframe, delta_time);

	for (size_t i = 0; i < astro_boy_matrices.size(); ++i)
	{
		if (astro_boy_tree[i].m_type == 1)
			astro_boy_joint_matrices.push_back(astro_boy_matrices[i] * get_ror_matrix4(astro_boy_tree[i].m_inverse));
	}

	astro_boy_skin->update_matrices(astro_boy_joint_matrices);
}

void get_mvp(ror::Matrix4f &out_model, ror::Matrix4f &out_view, ror::Matrix4f &out_projection)
{
	static float current_rotation = 0.0f;

	current_rotation = do_animate ? static_cast<float>(glfwGetTime() * 70.0f) : current_rotation;

	// Rotation around X to bring Y-Up
	auto rotation_x = ror::matrix4_rotation_around_x(ror::to_radians(-90.0f));

	auto translation = ror::matrix4_translation(0.0f, -3.0f, -10.0f);
	auto rotation_y  = ror::matrix4_rotation_around_y(ror::to_radians(current_rotation));
	out_projection   = ror::make_perspective(ror::to_radians(60.0f), aspect_ratio, 0.5f, 100.0f);
	out_view         = ror::make_look_at(ror::Vector3f(0.0f, 3.0f, 0.0f), ror::Vector3f(0.0f, 0.0f, -10.0f), ror::Vector3f(0.0f, 1.0f, 0.0f));

	out_model = translation * rotation_y * rotation_x;
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ror::Matrix4f model, view, projection;

	get_mvp(model, view, projection);

	auto mvp = projection * view * model;

	if (show_cube)
		cube->draw(mvp.m_values, GL_LINES);

	if (show_skin)
		astro_boy_skin->draw(model.m_values, view.m_values, projection.m_values, GL_TRIANGLES);

	if (show_skeleton)
		astro_boy_skeleton->draw(mvp.m_values, GL_LINES);
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
			do_animate = false;
			break;
		case GLFW_KEY_C:
			do_animate = true;
			break;
		case GLFW_KEY_G:
			show_skin = false;
			break;
		case GLFW_KEY_T:
			show_skin = true;
			break;
		case GLFW_KEY_Y:
			show_skeleton = false;
			break;
		case GLFW_KEY_H:
			show_skeleton = true;
			break;
		case GLFW_KEY_U:
			show_cube = false;
			break;
		case GLFW_KEY_J:
			show_cube = true;
			break;
		case GLFW_KEY_W:
			break;
		case GLFW_KEY_S:
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
