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

#include "astro_boy.hpp"
#include "geometry.hpp"
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

// Recursive function to get valid parent matrix, This is very unoptimised
// These matrices are calculated for each node, It should be cached instead, and have an iterative solution to it
ror::Matrix4f get_world_matrix(AstroBoyTreePtr a_node, unsigned int a_index)
{
	if (a_node[a_index].m_parent_id == -1)
		return get_ror_matrix4(a_node[a_index].m_transform);
	else
		return get_world_matrix(a_node, a_node[a_index].m_parent_id) * get_ror_matrix4(a_node[a_index].m_transform);
}

std::map<int, std::pair<int, ror::Matrix4f>> get_world_matrices_for_skeleton(AstroBoyTreePtr root, unsigned int joint_count)
{
	std::map<int, std::pair<int, ror::Matrix4f>> world_matrices;

	for (unsigned int i = 0; i < joint_count; ++i)
	{
		auto matrix = get_world_matrix(root, i);
		// auto wm = matrix * astro_boy_anim.bind_shape; // TODO: Do this once you have anything useful in bind_shape
		world_matrices[i] = std::make_pair(root[i].m_parent_id, matrix);
	}

	return world_matrices;
}

Geometry *get_lines_from_skeleton(std::map<int, std::pair<int, ror::Matrix4f>> &a_world_matrices,
								  const char *a_vertex_shader_src, const char *a_fragment_shader_src)
{
	std::vector<float>        vertices;
	std::vector<unsigned int> indices;

	int index = 0;

	for (auto &node : a_world_matrices)
	{
		auto origin = node.second.second.origin();
		vertices.push_back(origin.x);
		vertices.push_back(origin.y);
		vertices.push_back(origin.z);

		if (node.second.first == -1)
		{
			// Origin, can be excluded
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);
		}
		else
		{
			auto origin = a_world_matrices[node.second.first].second.origin();
			vertices.push_back(origin.x);
			vertices.push_back(origin.y);
			vertices.push_back(origin.z);
		}

		indices.push_back(index);
		indices.push_back(index + 1);

		index += 2;
	}

	return new Geometry(a_vertex_shader_src, a_fragment_shader_src,
						sizeof(float) * vertices.size(), vertices.data(),
						sizeof(unsigned int) * indices.size(), indices.data(), indices.size());
}
