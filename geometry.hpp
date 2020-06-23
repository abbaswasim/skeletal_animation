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

#pragma once

#include "CImg.h"
#include "gl_common.hpp"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <iostream>
#include <utility>
#include <vector>

#include "math/rormatrix4.hpp"
#include "math/rormatrix4_functions.hpp"

void check_gl_error(const char *file, int line)
{
	GLenum err(glGetError());
	while (err != GL_NO_ERROR)
	{
		std::string error;
		switch (err)
		{
			case GL_INVALID_OPERATION:
				error = "INVALID_OPERATION";
				break;
			case GL_INVALID_ENUM:
				error = "INVALID_ENUM";
				break;
			case GL_INVALID_VALUE:
				error = "INVALID_VALUE";
				break;
			case GL_OUT_OF_MEMORY:
				error = "OUT_OF_MEMORY";
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				error = "INVALID_FRAMEBUFFER_OPERATION";
				break;
		}

		std::cout << "GL_" << error.c_str() << " - " << file << ":" << line << std::endl;
		err = glGetError();
	}
}

GLuint compile_shaders(const char *a_vertex_shader_src, const char *a_fragment_shader_src)
{
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &a_vertex_shader_src, NULL);
	glCompileShader(vertex_shader);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &a_fragment_shader_src, NULL);
	glCompileShader(fragment_shader);

	int compiled_status = 0;

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled_status);

	if (compiled_status != 0)
	{
		std::cout << "Vertex shader compiled successfully." << std::endl;
	}
	else
	{
		char *log;
		int   length;

		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length);

		log = new char[length];
		glGetShaderInfoLog(vertex_shader, length, &length, log);

		std::cout << "Vertex shader compile log:" << std::endl;
		std::cout << log << std::endl;
		delete[] log;

		exit(0);
	}

	compiled_status = 0;

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled_status);

	if (compiled_status != 0)
	{
		std::cout << "Fragment shader compiled successfully." << std::endl;
	}
	else
	{
		char *log;
		int   length;

		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length);

		log = new char[length];
		glGetShaderInfoLog(fragment_shader, length, &length, log);

		std::cout << "Fragment shader compile log:" << std::endl;
		std::cout << log << std::endl;
		delete[] log;

		exit(0);
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	return program;
}

void read_texture_from_file(const char *a_file_name, unsigned char **a_data, unsigned int &a_width, unsigned int &a_height, unsigned int &a_bpp)
{
	cimg_library::CImg<unsigned char> src(a_file_name);
	// src.save("boy_10.ppm");

	unsigned int width  = src.width();
	unsigned int height = src.height();
	unsigned int bpp    = src.spectrum();

	a_width  = src.width();
	a_height = src.height();
	a_bpp    = src.spectrum();

	unsigned char *ptr = src.data();

	unsigned int size = width * height;

	unsigned char *mixed = new unsigned char[size * bpp];

	for (unsigned int i = 0; i < size; i++)
	{
		for (unsigned int j = 0; j < bpp; j++)
		{
			mixed[(i * bpp) + j] = ptr[i + (j * size)];
		}
	}

	*a_data = mixed;
}

GLuint create_texture(const char *a_file_name)
{
	unsigned char *data  = nullptr;
	unsigned int   width = 0, height = 0, bpp = 0;
	read_texture_from_file(a_file_name, &data, width, height, bpp);

	GLuint texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if (bpp == 2)
	{
		std::cout << "WARNING: Unsupported texture format, only support RGB/R format. " << a_file_name << std::endl;
		exit(1);
	}

	if (bpp == 1)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}

	glGenerateMipmap(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

class Geometry
{
  public:
	Geometry(){};

	Geometry(const char *a_vertex_shader_src, const char *a_fragment_shader_src, const char *a_texture_file_name,
			 unsigned int a_vertex_position_buffer_object_size, void *a_vertex_position_buffer_object,
			 unsigned int a_index_buffer_object_size, void *a_index_buffer_object, unsigned int a_primitivies_count)
	{
		this->m_program = compile_shaders(a_vertex_shader_src, a_fragment_shader_src);

		this->m_mvp_location = glGetUniformLocation(this->m_program, "model_view_projection");

		glGenVertexArrays(1, &this->m_vertex_array);
		glBindVertexArray(this->m_vertex_array);

		glGenBuffers(1, &this->m_vertex_position_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, a_vertex_position_buffer_object_size, a_vertex_position_buffer_object, GL_STATIC_DRAW);

		glGenBuffers(1, &this->m_index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, a_index_buffer_object_size, a_index_buffer_object, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, nullptr);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, reinterpret_cast<void *>(sizeof(float) * 3));

		if (a_texture_file_name != nullptr)
			this->m_texture = create_texture(a_texture_file_name);

		this->m_primitives_count = a_primitivies_count;

		check_gl_error(__FILE__, __LINE__);
	}

	void bind_me(const GLfloat *mvp)
	{
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);

		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_position_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, nullptr);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, reinterpret_cast<void *>(sizeof(float) * 3));

		glUniformMatrix4fv(m_mvp_location, 1, GL_FALSE, mvp);
		check_gl_error(__FILE__, __LINE__);

		if (this->m_texture != -1)
		{
			std::printf("Texture is = %d\n", this->m_texture);
			glBindTexture(GL_TEXTURE_2D, this->m_texture);
		}

		check_gl_error(__FILE__, __LINE__);
	}
	void unbind_me()
	{
		glUseProgram(0);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		check_gl_error(__FILE__, __LINE__);
	}

	void draw(const GLfloat *mvp, GLint prim)
	{
		bind_me(mvp);
		glDrawElements(prim, m_primitives_count, GL_UNSIGNED_INT, nullptr);
		unbind_me();
		check_gl_error(__FILE__, __LINE__);
	}

  private:
	GLuint m_program = -1;
	GLuint m_texture = -1;
	GLint  m_mvp_location;
	GLint  m_position_location;
	GLuint m_vertex_position_buffer;
	GLuint m_index_buffer;
	GLuint m_vertex_array;
	GLuint m_primitives_count;
};

Geometry *create_cube(float a_size, ror::Vector3f a_origin, const char *a_vertex_shader_src, const char *a_fragment_shader_src)
{
	std::vector<float>        vertices;
	std::vector<unsigned int> indices;

	ror::Vector3f size(a_size, a_size, a_size);
	ror::Vector3f color(0.0f, 1.0f, 0.0f);

	vertices.push_back(size.x + a_origin.x);
	vertices.push_back(size.y + a_origin.y);
	vertices.push_back(size.z + a_origin.z);
	vertices.push_back(color.x);
	vertices.push_back(color.y);
	vertices.push_back(color.z);

	vertices.push_back(-size.x + a_origin.x);
	vertices.push_back(size.y + a_origin.y);
	vertices.push_back(size.z + a_origin.z);
	vertices.push_back(color.x);
	vertices.push_back(color.y);
	vertices.push_back(color.z);

	vertices.push_back(-size.x + a_origin.x);
	vertices.push_back(-size.y + a_origin.y);
	vertices.push_back(size.z + a_origin.z);
	vertices.push_back(color.x);
	vertices.push_back(color.y);
	vertices.push_back(color.z);

	vertices.push_back(size.x + a_origin.x);
	vertices.push_back(-size.y + a_origin.y);
	vertices.push_back(size.z + a_origin.z);
	vertices.push_back(color.x);
	vertices.push_back(color.y);
	vertices.push_back(color.z);

	vertices.push_back(-size.x + a_origin.x);
	vertices.push_back(-size.y + a_origin.y);
	vertices.push_back(-size.z + a_origin.z);
	vertices.push_back(color.x);
	vertices.push_back(color.y);
	vertices.push_back(color.z);

	vertices.push_back(size.x + a_origin.x);
	vertices.push_back(-size.y + a_origin.y);
	vertices.push_back(-size.z + a_origin.z);
	vertices.push_back(color.x);
	vertices.push_back(color.y);
	vertices.push_back(color.z);

	vertices.push_back(size.x + a_origin.x);
	vertices.push_back(size.y + a_origin.y);
	vertices.push_back(-size.z + a_origin.z);
	vertices.push_back(color.x);
	vertices.push_back(color.y);
	vertices.push_back(color.z);

	vertices.push_back(-size.x + a_origin.x);
	vertices.push_back(size.y + a_origin.y);
	vertices.push_back(-size.z + a_origin.z);
	vertices.push_back(color.x);
	vertices.push_back(color.y);
	vertices.push_back(color.z);

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(3);
	indices.push_back(0);

	indices.push_back(4);
	indices.push_back(5);
	indices.push_back(5);
	indices.push_back(6);
	indices.push_back(6);
	indices.push_back(7);
	indices.push_back(7);
	indices.push_back(4);

	indices.push_back(0);
	indices.push_back(6);
	indices.push_back(1);
	indices.push_back(7);
	indices.push_back(2);
	indices.push_back(4);
	indices.push_back(3);
	indices.push_back(5);

	return new Geometry(a_vertex_shader_src, a_fragment_shader_src, nullptr,
						sizeof(float) * vertices.size(), vertices.data(),
						sizeof(unsigned int) * indices.size(), indices.data(), indices.size());
}

class AnimatedGeometry
{
  public:
	AnimatedGeometry(){};

	AnimatedGeometry(const char *a_vertex_shader_src, const char *a_fragment_shader_src, const char *a_texture_file_name,
					 unsigned int a_vertex_position_buffer_object_size, void *a_vertex_position_buffer_object,
					 unsigned int a_vertex_normal_buffer_object_size, void *a_vertex_normal_buffer_object,
					 unsigned int a_vertex_uv_buffer_object_size, void *a_vertex_uv_buffer_object,
					 unsigned int a_vertex_weight_buffer_object_size, void *a_vertex_weight_buffer_object,
					 unsigned int a_vertex_joint_buffer_object_size, void *a_vertex_joint_buffer_object,
					 unsigned int a_index_buffer_object_size, void *a_index_buffer_object,
					 unsigned int a_primitivies_count, unsigned int a_joints_count = 44)
	{
		this->m_program = compile_shaders(a_vertex_shader_src, a_fragment_shader_src);

		check_gl_error(__FILE__, __LINE__);
		this->m_model_location      = glGetUniformLocation(this->m_program, "model");
		this->m_view_location       = glGetUniformLocation(this->m_program, "view");
		this->m_projection_location = glGetUniformLocation(this->m_program, "projection");

		this->m_texture_location = glGetUniformLocation(this->m_program, "diffuse_texture");

		this->m_uniform_block_index = glGetUniformBlockIndex(this->m_program, "joint_matrices");

		check_gl_error(__FILE__, __LINE__);
		glGenBuffers(1, &this->m_joint_matrices);

		check_gl_error(__FILE__, __LINE__);
		glBindBuffer(GL_UNIFORM_BUFFER, this->m_joint_matrices);
		glBufferData(GL_UNIFORM_BUFFER, a_joints_count * sizeof(ror::Matrix4f), nullptr, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		check_gl_error(__FILE__, __LINE__);
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, this->m_joint_matrices, 0, a_joints_count * sizeof(ror::Matrix4f));

		check_gl_error(__FILE__, __LINE__);
		if (this->m_uniform_block_index != -1)
			glUniformBlockBinding(this->m_program, this->m_uniform_block_index, 0);

		check_gl_error(__FILE__, __LINE__);
		glGenVertexArrays(1, &this->m_vertex_array);
		glBindVertexArray(this->m_vertex_array);

		glGenBuffers(1, &this->m_vertex_position_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, a_vertex_position_buffer_object_size, a_vertex_position_buffer_object, GL_STATIC_DRAW);

		glGenBuffers(1, &this->m_vertex_normal_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_normal_buffer);
		glBufferData(GL_ARRAY_BUFFER, a_vertex_normal_buffer_object_size, a_vertex_normal_buffer_object, GL_STATIC_DRAW);

		glGenBuffers(1, &this->m_vertex_uv_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_uv_buffer);
		glBufferData(GL_ARRAY_BUFFER, a_vertex_uv_buffer_object_size, a_vertex_uv_buffer_object, GL_STATIC_DRAW);

		glGenBuffers(1, &this->m_vertex_weight_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_weight_buffer);
		glBufferData(GL_ARRAY_BUFFER, a_vertex_weight_buffer_object_size, a_vertex_weight_buffer_object, GL_STATIC_DRAW);

		glGenBuffers(1, &this->m_vertex_joint_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_joint_buffer);
		glBufferData(GL_ARRAY_BUFFER, a_vertex_joint_buffer_object_size, a_vertex_joint_buffer_object, GL_STATIC_DRAW);

		glGenBuffers(1, &this->m_index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, a_index_buffer_object_size, a_index_buffer_object, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);        // Position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(1);        // Normal
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(2);        // UV
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(3);        // Weights
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(4);        // Joints
		glVertexAttribIPointer(4, 3, GL_UNSIGNED_INT, 0, nullptr);

		if (a_texture_file_name != nullptr)
			this->m_texture = create_texture(a_texture_file_name);

		check_gl_error(__FILE__, __LINE__);
		this->m_primitives_count = a_primitivies_count;
	}

	void bind_me(const GLfloat *model, const GLfloat *view, const GLfloat *projection)
	{
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);

		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_position_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_normal_buffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_uv_buffer);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_weight_buffer);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_joint_buffer);
		glVertexAttribIPointer(4, 3, GL_UNSIGNED_INT, 0, nullptr);

		if (this->m_texture != -1)
			glBindTexture(GL_TEXTURE_2D, this->m_texture);

		glUniformMatrix4fv(m_model_location, 1, GL_FALSE, model);
		glUniformMatrix4fv(m_view_location, 1, GL_FALSE, view);
		glUniformMatrix4fv(m_projection_location, 1, GL_FALSE, projection);

		if (this->m_texture != -1 && this->m_texture_location != -1)
			glUniform1i(this->m_texture_location, 0);

		glBindBuffer(GL_UNIFORM_BUFFER, this->m_joint_matrices);

		check_gl_error(__FILE__, __LINE__);
	}

	void unbind_me()
	{
		glUseProgram(0);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		check_gl_error(__FILE__, __LINE__);
	}

	void update_matrices(const std::vector<ror::Matrix4f> &a_matrices)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, this->m_joint_matrices);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, a_matrices.size() * sizeof(ror::Matrix4f), a_matrices.data());
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void draw(const GLfloat *model, const GLfloat *view, const GLfloat *projection, GLint prim)
	{
		bind_me(model, view, projection);
		glDrawElements(prim, m_primitives_count, GL_UNSIGNED_INT, nullptr);
		unbind_me();
		check_gl_error(__FILE__, __LINE__);
	}

  private:
	GLuint m_program          = -1;
	GLint  m_texture          = -1;
	GLint  m_texture_location = -1;
	GLint  m_model_location;
	GLint  m_view_location;
	GLint  m_projection_location;
	GLuint m_vertex_position_buffer;
	GLuint m_vertex_normal_buffer;
	GLuint m_vertex_uv_buffer;
	GLuint m_vertex_weight_buffer;
	GLuint m_vertex_joint_buffer;
	GLuint m_index_buffer;

	GLuint m_uniform_block_index = -1;
	GLuint m_joint_matrices      = -1;

	GLuint m_vertex_array;
	GLuint m_primitives_count;
};
