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

// Simple helper to create those astro_boy_[geometry,animation,skeleton].hpp files from raw collada data
// All the data in astro_boy_geometry_from_collada.hpp is copy pasted from astroBoy_walk_Max.dae

// To regenerate the headers again use the following command
// clang++ -fsanitize=undefined geometry_generator.cpp -o geom && ./geom

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#include "astro_boy_geometry_from_collada.hpp"

typedef unsigned long long uint64_t;

uint64_t hash(unsigned int a, unsigned int b, unsigned int c, unsigned int n)
{
	return a * (n + 1) * (n + 1) + b * (n + 1) + c;
}

template <typename T>
void write_vertex_array(std::ofstream &header_file, std::vector<T> data, unsigned int components, std::string name, std::string type)
{
	header_file << "\nconst unsigned int " << name << "_array_count = astro_boy_vertex_count * " << components
				<< ";\n"
				<< type << " " << name << "[" << name << "_array_count] = {";

	for (size_t i = 0; i < data.size(); ++i)
	{
		header_file << data[i];
		if (i != data.size() - 1)
			header_file << ",";
	}

	header_file << "};\n";
}

template <typename T>
void write_index_array(std::ofstream &header_file, std::vector<T> data, unsigned int components, std::string name, std::string type)
{
	header_file << "\nconst unsigned int " << name << "_array_count = " << data.size()
				<< ";\n"
				<< type << " " << name << "[" << name << "_array_count] = {";

	for (size_t i = 0; i < data.size(); ++i)
	{
		header_file << data[i];
		if (i != data.size() - 1)
			header_file << ",";
	}

	header_file << "};\n\n";
}

union KeyframeInt
{
	float        m_float;
	unsigned int m_int;
};

int main(int argc, char *argv[])
{
	// Max index I will have in the triangle list
	unsigned int s = std::max(std::max(astro_boy_positions_array_count, astro_boy_texture_coordinates_array_count), astro_boy_normals_array_count);

	// Lets flatten weights and joints array indices from <vcount> and <v> to some sane vectors of weights {w0, w1, w2 ....} and joints {j0, j1, j2 ....}
	// This way each vertex now have at most max_joints weights and joints indices
	std::vector<int> weights_flat;
	std::vector<int> joints_flat;

	weights_flat.reserve(s * 4);
	joints_flat.reserve(s * 4);

	unsigned int max_joints        = 0;
	int          max_joints_index  = 0;
	int          max_weights_index = 0;

	// Find maximum number of weights/joints
	for (size_t i = 0; i < astro_boy_joints_weights_count_array_count; ++i)
	{
		max_joints = std::max(max_joints, astro_boy_joints_weights_count_array[i]);
	}

	assert(max_joints <= 4);
	assert(astro_boy_joints_weights_count_array_count == astro_boy_positions_count);

	unsigned int v_index = 0;
	for (size_t i = 0; i < astro_boy_joints_weights_count_array_count; ++i)
	{
		auto amount    = astro_boy_joints_weights_count_array[i];
		int  weight[4] = {-1, -1, -1, -1};        // Maximum maximum joints influence allowed
		int  joint[4]  = {-1, -1, -1, -1};        // Maximum maximum joints influence allowed

		assert(amount <= max_joints);

		for (size_t j = 0; j < amount; ++j)
		{
			joint[j]  = astro_boy_joints_weights_values_array[v_index + j * 2 + 0];        // Joint ID
			weight[j] = astro_boy_joints_weights_values_array[v_index + j * 2 + 1];        // Weight ID

			max_joints_index  = std::max(max_joints_index, joint[j]);
			max_weights_index = std::max(max_weights_index, weight[j]);
		}

		v_index += amount * 2;

		for (size_t j = 0; j < max_joints; ++j)
		{
			weights_flat.push_back(weight[j]);
			joints_flat.push_back(joint[j]);
		}
	}

	assert((weights_flat.size() / max_joints) == astro_boy_positions_count);
	assert((joints_flat.size() / max_joints) == astro_boy_positions_count);

	assert((max_joints_index * max_joints) < astro_boy_positions_array_count);
	assert((max_weights_index * max_joints) < astro_boy_positions_array_count);

	unsigned int pmax = 0;
	unsigned int tmax = 0;
	unsigned int nmax = 0;

	for (size_t i = 0; i < astro_boy_triangles_count; i += 3)
	{
		pmax = std::max(pmax, astro_boy_triangles[i + 0]);
		nmax = std::max(nmax, astro_boy_triangles[i + 1]);
		tmax = std::max(tmax, astro_boy_triangles[i + 2]);
	}

	std::cout << "Max Ps = " << pmax << std::endl;
	std::cout << "Max Ns = " << nmax << std::endl;
	std::cout << "Max Ts = " << tmax << std::endl;

	std::vector<float>        positions;
	std::vector<float>        normals;
	std::vector<float>        uvs;
	std::vector<unsigned int> indices;
	std::vector<float>        weights;
	std::vector<int>          joints;

	positions.reserve(s * 2);
	normals.reserve(s * 2);
	uvs.reserve(s * 2);
	weights.reserve(s * 4);
	joints.reserve(s * 4);
	indices.reserve(s);

	std::unordered_map<uint64_t, unsigned int> indices_map;

	unsigned int running_index = 0;

	for (size_t i = 0; i < astro_boy_triangles_count; i += 3)
	{
		auto p = astro_boy_triangles[i + 0] * 3;
		auto n = astro_boy_triangles[i + 1] * 3;
		auto t = astro_boy_triangles[i + 2] * 3;
		auto w = astro_boy_triangles[i];        // an alias for P

		assert(p <= s && t <= s && n <= s);

		auto h = hash(p, t, n, s);

		auto ind = indices_map.find(h);

		if (ind != indices_map.end())
		{
			indices.emplace_back(ind->second);
		}
		else
		{
			for (size_t j = 0; j < 3; ++j)
			{
				positions.push_back(astro_boy_positions[p + j]);
				normals.push_back(astro_boy_normals[n + j]);
			}

			uvs.push_back(astro_boy_texture_coordinates[t + 0]);
			uvs.push_back(astro_boy_texture_coordinates[t + 1]);

			for (size_t j = 0; j < max_joints; ++j)
			{
				weights.push_back((weights_flat[w * max_joints + j] == -1 ? 0.0f : astro_boy_weights[weights_flat[w * max_joints + j]]));
				joints.push_back((joints_flat[w * max_joints + j]) == -1 ? 0 : joints_flat[w * max_joints + j]);
			}

			indices.emplace_back(running_index);
			indices_map[h] = running_index++;
		}
	}

	// Now lets start writing out
	std::cout << "Size of positions=" << positions.size() << std::endl;
	std::cout << "Size of normals=" << normals.size() << std::endl;
	std::cout << "Size of uvs=" << uvs.size() << std::endl;
	std::cout << "Size of max_joints=" << max_joints << std::endl;
	std::cout << "Size of weights=" << weights.size() << std::endl;
	std::cout << "Size of joints=" << joints.size() << std::endl;
	std::cout << "Size of triangles_before=" << astro_boy_triangles_count / 9 << std::endl;
	std::cout << "Size of indices=" << indices.size() / 3 << std::endl;

	std::cout << "Writing out astro_boy_geometry.hpp\n";

	assert((positions.size() / 3) == (normals.size() / 3) && (normals.size() / 3) == (uvs.size() / 2));

	{
		std::ofstream header_file("astro_boy_geometry.hpp");

		header_file << "const unsigned int astro_boy_vertex_count = " << positions.size() / 3 << ";\n";

		write_vertex_array(header_file, positions, 3, "astro_boy_positions", "float");
		write_vertex_array(header_file, normals, 3, "astro_boy_normals", "float");
		write_vertex_array(header_file, uvs, 2, "astro_boy_uvs", "float");
		write_vertex_array(header_file, weights, max_joints, "astro_boy_weights", "float");
		write_vertex_array(header_file, joints, max_joints, "astro_boy_joints", "int");

		// Now write out indices for all triangles
		header_file << "const unsigned int astro_boy_triangles_count = " << indices.size() / 3 << ";\n";

		write_index_array(header_file, indices, 1, "astro_boy_indices", "unsigned int");

		header_file.close();
	}

	{
		std::map<int, float> known_keyframes;
		for (size_t i = 0; i < astro_boy_animations_count; ++i)
		{
			assert(astro_boy_animations[i].keyframes.size() == 32);

			for (size_t j = 0; j < astro_boy_animations[i].keyframes.size(); j += 2)
			{
				assert(astro_boy_animations[i].keyframes[j].size() == 36 || astro_boy_animations[i].keyframes[j].size() == 2);

				if (astro_boy_animations[i].keyframes[j].size() == 2)
				{
					KeyframeInt kf0;
					kf0.m_float = astro_boy_animations[i].keyframes[j + 1][0];

					KeyframeInt kf1;
					kf1.m_float = astro_boy_animations[i].keyframes[j + 1][1];

					assert(kf0.m_float == kf1.m_float);
					assert(kf0.m_int == kf1.m_int);
				}

				for (size_t k = 0; k < astro_boy_animations[i].keyframes[j].size(); ++k)
				{
					KeyframeInt kf;
					kf.m_float                = astro_boy_animations[i].keyframes[j][k];
					known_keyframes[kf.m_int] = kf.m_float;
				}
			}
		}

		std::cout << "Writing out astro_boy_animation.hpp\n";

		// Read and write out animation data
		std::ofstream header_file("astro_boy_animation.hpp");

		header_file << std::fixed << std::setprecision(6) << "#include <astro_boy_skeleton.hpp> \n\nunsigned int astro_boy_animation_keyframes_count = " << known_keyframes.size() << ";";
		header_file << "\nstd::vector<float> astro_boy_animation_keyframe_times = {";

		unsigned int c = 0;
		for (auto &key : known_keyframes)
		{
			header_file << key.second;
			if (c++ != known_keyframes.size() - 1)
				header_file << ",";
		}

		header_file << "}; \n\n// Map by joint id, with matrices for all keyframes\nstd::map<unsigned int, std::vector<ColladaMatrix>> astro_boy_animation_keyframe_matrices = {";

		for (size_t i = 0; i < astro_boy_animations_count; ++i)
		{
			assert(astro_boy_animations[i].keyframes.size() == 32);

			auto itr = std::find(std::begin(astro_boy_tree_collada), std::end(astro_boy_tree_collada), astro_boy_animations[i]);
			if (itr != std::end(astro_boy_tree_collada))
			{
				header_file << "\n\t{" << itr->m_index << ",\n\t\t{";

				for (size_t j = 0; j < known_keyframes.size(); ++j)
				{
					header_file << "\n\t\t\t{";

					for (size_t l = 0; l < astro_boy_animations[i].keyframes.size(); l += 2)        // Basically 16x loop
					{

						unsigned int matrix_index = 0;
						if (astro_boy_animations[i].keyframes[l + 1].size() != 2)
							matrix_index = j;

						if (l + 1 == 31)
							header_file << 1.0f; // Collada bug workaround
						else
							header_file << astro_boy_animations[i].keyframes[l + 1][matrix_index];

						if (l < astro_boy_animations[i].keyframes.size() - 2)
							header_file << ",";
					}

					header_file << "}";
					if (j < known_keyframes.size() - 1)
						header_file << ",";
				}

				// std::cout << "End of list\n";
				header_file << "\n\t\t}\n\t}";        // One map matrix entry finish
				if (i < astro_boy_animations_count - 1)
					header_file << ",";
			}
			else
			{
				assert(0);        // This shouldn't happen
			}
		}

		header_file << "\n};";
	}

	return 0;
}
