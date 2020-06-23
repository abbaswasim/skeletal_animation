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

#include "astro_boy_animation.hpp"
#include "astro_boy_geometry.hpp"
#include "geometry.hpp"
#include "math/rorvector3.hpp"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <map>

ror::Matrix4f get_ror_matrix4(ColladaMatrix &mat)
{
	ror::Matrix4f matrix;

	for (int i = 0; i < 16; ++i)
		matrix.m_values[i] = mat.v[i];

	// Since collada matrices are colum_major BUT they are provided in row-major for readibility, you need to transpose when you read
	// https://www.khronos.org/files/collada_spec_1_4.pdf 5-77 "Matrices in COLLADA are column matrices in the mathematical sense. These matrices are written in row- major order to aid the human reader. See the example."
	return matrix.transposed();
}

ror::Matrix4f get_animated_transform(AstroBoyTreePtr a_node, unsigned int a_index, double a_keyframe, double a_accumulate_time)
{
	if (astro_boy_animation_keyframe_matrices.find(a_index) != astro_boy_animation_keyframe_matrices.end())
	{
		assert(a_accumulate_time + 1 < astro_boy_animation_keyframes_count);

		float a = astro_boy_animation_keyframe_times[a_accumulate_time];
		float b = astro_boy_animation_keyframe_times[a_accumulate_time + 1];
		float t = a_keyframe / (b - a);

		return ror::matrix4_interpolate(get_ror_matrix4(astro_boy_animation_keyframe_matrices[a_index][a_accumulate_time]),
										get_ror_matrix4(astro_boy_animation_keyframe_matrices[a_index][a_accumulate_time + 1]), t);
	}
	else
	{
		return get_ror_matrix4(a_node[a_index].m_transform);
	}

	return ror::Matrix4f();
}

// Recursive function to get valid parent matrix, This is very unoptimised
// These matrices are calculated for each node, It should be cached instead, and have an iterative solution to it
ror::Matrix4f get_world_matrix(AstroBoyTreePtr a_node, unsigned int a_index)
{
	if (a_node[a_index].m_parent_id == -1)
		return get_ror_matrix4(a_node[a_index].m_transform);
	else
		return get_world_matrix(a_node, a_node[a_index].m_parent_id) * get_ror_matrix4(a_node[a_index].m_transform);
}

// Recursive function to get valid parent matrix, This is very unoptimised
// These matrices are calculated for each node, It should be cached instead, and have an iterative solution to it
ror::Matrix4f get_world_matrix_animated(AstroBoyTreePtr a_node, unsigned int a_index, double a_keyframe, double a_accumulate_time)
{
	if (a_node[a_index].m_parent_id == -1)
		return get_animated_transform(a_node, a_index, a_keyframe, a_accumulate_time);
	else
		return get_world_matrix_animated(a_node, a_node[a_index].m_parent_id, a_keyframe, a_accumulate_time) * get_animated_transform(a_node, a_index, a_keyframe, a_accumulate_time);
}

std::map<int, std::pair<int, ror::Matrix4f>> get_world_matrices_for_skeleton(AstroBoyTreePtr root, unsigned int joint_count)
{
	std::map<int, std::pair<int, ror::Matrix4f>> world_matrices;

	for (unsigned int i = 0; i < joint_count; ++i)
	{
		auto matrix       = get_world_matrix(root, i);
		matrix            = matrix * get_ror_matrix4(astro_boy_skeleton_bind_shape_matrix);        // at the moment bind_shape is identity
		world_matrices[i] = std::make_pair(root[i].m_parent_id, matrix);
	}

	return world_matrices;
}

std::vector<ror::Matrix4f> get_world_matrices_for_skinning(AstroBoyTreePtr root, unsigned int joint_count, double a_keyframe)
{
	// Note this is very specific to AstroBoy
	static double a_accumulate_time = 0.0;
	const double  pf                = 1.166670 / 36.0;

	static int a_time = 0;

	a_accumulate_time += a_keyframe;
	a_time = a_accumulate_time / pf;

	if (a_accumulate_time > 1.66670 || (a_time > astro_boy_animation_keyframes_count - 5))        // Last 5 frames don't quite work with the animation loop, so ignored
	{
		a_accumulate_time = 0.0;
		a_time            = 0;
	}

	std::vector<ror::Matrix4f> world_matrices;
	world_matrices.reserve(joint_count);

	for (unsigned int i = 0; i < joint_count; ++i)
	{
		auto matrix = get_world_matrix_animated(root, i, a_keyframe, a_time);
		matrix      = matrix * get_ror_matrix4(astro_boy_skeleton_bind_shape_matrix);        // at the moment bind_shape is identity
		world_matrices.push_back(matrix);
	}

	return world_matrices;
}

void add_vector(std::vector<float> &a_vertices, ror::Vector3f &&a_position, ror::Vector3f &&a_color,
				std::vector<unsigned int> &a_indices, unsigned int a_index)
{
	a_vertices.emplace_back(a_position.x);
	a_vertices.emplace_back(a_position.y);
	a_vertices.emplace_back(a_position.z);

	a_vertices.emplace_back(a_color.x);
	a_vertices.emplace_back(a_color.y);
	a_vertices.emplace_back(a_color.z);

	// Write out indices
	a_indices.push_back(a_index);
}

Geometry *get_lines_from_skeleton(std::map<int, std::pair<int, ror::Matrix4f>> &a_world_matrices,
								  const char *a_vertex_shader_src, const char *a_fragment_shader_src)
{
	std::vector<float>        vertices;
	std::vector<unsigned int> indices;

	ror::Vector4f skeleton_color{0.5f, 0.5f, 0.5f, 0.5f};

	ror::Vector4f red_color{0.4f, 0.0f, 0.0f, 0.5f};
	ror::Vector4f green_color{0.0f, 0.4f, 0.0f, 0.5f};
	ror::Vector4f blue_color{0.0f, 0.0f, 0.4f, 0.5f};

	int index = 0;

	// Add coordinate axis
	auto o = ror::Vector3f(0.0f, 0.0f, 0.0f);
	auto x = ror::Vector3f(1.0f, 0.0f, 0.0f);
	auto y = ror::Vector3f(0.0f, 1.0f, 0.0f);
	auto z = ror::Vector3f(0.0f, 0.0f, 1.0f);

	add_vector(vertices, ror::Vector3f(o), ror::Vector3f(red_color * 2.5f), indices, index++);
	add_vector(vertices, ror::Vector3f(o + x), ror::Vector3f(red_color * 2.5f), indices, index++);

	add_vector(vertices, ror::Vector3f(o), ror::Vector3f(green_color * 2.5f), indices, index++);
	add_vector(vertices, ror::Vector3f(o + y), ror::Vector3f(green_color * 2.5f), indices, index++);

	add_vector(vertices, ror::Vector3f(o), ror::Vector3f(blue_color * 2.5f), indices, index++);
	add_vector(vertices, ror::Vector3f(o + z), ror::Vector3f(blue_color * 2.5f), indices, index++);

	float origin_scale = 0.3f;

	for (auto &node : a_world_matrices)
	{
		// Write out joint lines
		add_vector(vertices, node.second.second.origin(), ror::Vector3f(skeleton_color), indices, index++);

		if (node.second.first == -1)
		{
			// Origin, can be excluded
			add_vector(vertices, ror::Vector3f(0.0f, 0.0f, 0.0f), ror::Vector3f(skeleton_color), indices, index++);
		}
		else
		{
			add_vector(vertices, a_world_matrices[node.second.first].second.origin(), ror::Vector3f(skeleton_color), indices, index++);
		}

		// Write out coordinate axis
		auto x_axis = node.second.second.x_axis();
		auto y_axis = node.second.second.y_axis();
		auto z_axis = node.second.second.z_axis();

		x_axis.normalize();
		y_axis.normalize();
		z_axis.normalize();

		x_axis *= origin_scale;
		y_axis *= origin_scale;
		z_axis *= origin_scale;

		auto origin = (node.second.second.origin());

		add_vector(vertices, ror::Vector3f(origin), ror::Vector3f(red_color), indices, index++);
		add_vector(vertices, ror::Vector3f(origin + x_axis), ror::Vector3f(red_color), indices, index++);

		add_vector(vertices, ror::Vector3f(origin), ror::Vector3f(blue_color), indices, index++);
		add_vector(vertices, ror::Vector3f(origin + y_axis), ror::Vector3f(blue_color), indices, index++);

		add_vector(vertices, ror::Vector3f(origin), ror::Vector3f(green_color), indices, index++);
		add_vector(vertices, ror::Vector3f(origin + z_axis), ror::Vector3f(green_color), indices, index++);
	}

	return new Geometry(a_vertex_shader_src, a_fragment_shader_src, nullptr,
						sizeof(float) * vertices.size(), vertices.data(),
						sizeof(unsigned int) * indices.size(), indices.data(), indices.size());
}
