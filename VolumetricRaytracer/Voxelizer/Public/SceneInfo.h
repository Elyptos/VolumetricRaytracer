/*
	Copyright (c) 2020 Thomas Schöngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

#pragma once
#include "Vector.h"
#include "Quat.h"
#include "AABB.h"
#include <string>
#include <vector>
#include <boost/unordered_map.hpp>

namespace VolumeRaytracer
{
	namespace Voxelizer
	{
		struct VVertex
		{
			VVector Position;
			VVector Normal;
		};

		struct VMeshInfo
		{
			std::vector<VVertex> Vertices;
			std::vector<size_t> Indices;
			VAABB Bounds;
		};

		struct VObjectInfo
		{
			std::string MeshID;
			VVector Position;
			VVector Scale;
			VQuat Rotation;
		};

		struct VSceneInfo
		{
			boost::unordered_map<std::string, VMeshInfo> Meshes;
			std::vector<VObjectInfo> Objects;
		};
	}
}