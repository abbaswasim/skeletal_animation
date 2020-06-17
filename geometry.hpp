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

#include "gl_common.hpp"
#include <GLFW/glfw3.h>

#include "math/rormatrix4_functions.hpp"

class Geometry
{
  public:
	Geometry(){};

	Geometry(const char *a_vertex_shader_src, const char *a_fragment_shader_src,
			 unsigned int a_vertex_buffer_object_size, void *a_vertex_buffer_object,
			 unsigned int a_index_buffer_object_size, void *a_index_buffer_object, unsigned int a_primitivies_count)
	{
		GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_shader, 1, &a_vertex_shader_src, NULL);
		glCompileShader(vertex_shader);

		GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment_shader, 1, &a_fragment_shader_src, NULL);
		glCompileShader(fragment_shader);

		// TODO: Check compile status

		this->m_program = glCreateProgram();
		glAttachShader(this->m_program, vertex_shader);
		glAttachShader(this->m_program, fragment_shader);
		glLinkProgram(this->m_program);

		this->m_mvp_location      = glGetUniformLocation(this->m_program, "model_view_projection");
		this->m_position_location = glGetAttribLocation(this->m_program, "position");
		this->m_color_location    = glGetUniformLocation(this->m_program, "color");

		glGenVertexArrays(1, &this->m_vertex_array);
		glBindVertexArray(this->m_vertex_array);

		glGenBuffers(1, &this->m_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, a_vertex_buffer_object_size, a_vertex_buffer_object, GL_STATIC_DRAW);

		glGenBuffers(1, &this->m_index_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_index_buffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, a_index_buffer_object_size, a_index_buffer_object, GL_STATIC_DRAW);

		glEnableVertexAttribArray(this->m_position_location);
		glVertexAttribPointer(this->m_position_location, 3, GL_FLOAT, GL_FALSE, 0, 0);

		this->m_primitives_count = a_primitivies_count;
	}

	void bind_me(const GLfloat *mvp, float *color)
	{
		glUseProgram(m_program);
		glBindVertexArray(m_vertex_array);
		glUniformMatrix4fv(m_mvp_location, 1, GL_FALSE, mvp);
		glUniform4fv(m_color_location, 1, color);
	}
	void unbind_me()
	{
		glUseProgram(0);
		glBindVertexArray(0);
	}

	void draw(const GLfloat *mvp, GLint prim, float *color)
	{
		bind_me(mvp, color);
		glDrawElements(prim, m_primitives_count, GL_UNSIGNED_INT, nullptr);
		unbind_me();
	}

  private:
	GLuint m_program;
	GLint  m_mvp_location;
	GLint  m_position_location;
	GLint  m_color_location;
	GLuint m_vertex_buffer;
	GLuint m_index_buffer;
	GLuint m_vertex_array;
	GLuint m_primitives_count;
};

Geometry *create_cube(float a_size, const char *a_vertex_shader_src, const char *a_fragment_shader_src)
{
	std::vector<float>        vertices;
	std::vector<unsigned int> indices;

	ror::Vector3f size(a_size, a_size, a_size);

	vertices.push_back(size.x);
	vertices.push_back(size.y);
	vertices.push_back(size.z);

	vertices.push_back(-size.x);
	vertices.push_back(size.y);
	vertices.push_back(size.z);

	vertices.push_back(-size.x);
	vertices.push_back(-size.y);
	vertices.push_back(size.z);

	vertices.push_back(size.x);
	vertices.push_back(-size.y);
	vertices.push_back(size.z);

	vertices.push_back(-size.x);
	vertices.push_back(-size.y);
	vertices.push_back(-size.z);

	vertices.push_back(size.x);
	vertices.push_back(-size.y);
	vertices.push_back(-size.z);

	vertices.push_back(size.x);
	vertices.push_back(size.y);
	vertices.push_back(-size.z);

	vertices.push_back(-size.x);
	vertices.push_back(size.y);
	vertices.push_back(-size.z);

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

	return new Geometry(a_vertex_shader_src, a_fragment_shader_src,
						sizeof(float) * vertices.size(), vertices.data(),
						sizeof(unsigned int) * indices.size(), indices.data(), indices.size());
}
