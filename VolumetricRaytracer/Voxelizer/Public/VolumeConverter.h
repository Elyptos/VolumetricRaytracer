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
#include "Object.h"
#include "SceneInfo.h"

namespace VolumeRaytracer
{
	namespace Voxel
	{
		class VVoxelVolume;
	}

	namespace Voxelizer
	{
		struct VVertexCellIndex
		{
		public:
			VVertexCellIndex(){}
			VVertexCellIndex(const unsigned int& x, const unsigned int& y, const unsigned int& z)
			: X(x),
			Y(y),
			Z(z){}

			int X;
			int Y;
			int Z;
		};

		struct VTriangle
		{
		public:
			VVector V1;
			VVector V2;
			VVector V3;
			VVector Normal;
			VVector Mid;
		};

		class VVolumeConverter
		{
		public:
			static VObjectPtr<Voxel::VVoxelVolume> ConvertMeshInfoToVoxelVolume(const VMeshInfo& meshInfo);

		private:
			static void VoxelizeTriangle(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v1, const VVertex& v2, const VVertex& v3);

			static VVertexCellIndex GetCellIndex(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v);

			static VVector GetTriangleNormal(const VVertex& v1, const VVertex& v2, const VVertex& v3);
			static VVector GetTriangleMidpoint(const VVertex& v1, const VVertex& v2, const VVertex& v3);

			static void UpdateCellDensity(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertexCellIndex& cellIndex, const VTriangle& triangle);
			
			static bool IsPointOnTriangleIfProjected(const VVector& relToTrianglePoint, const VTriangle& triangle);
		};
	}
}