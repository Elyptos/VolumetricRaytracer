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

#include "VolumeConverter.h"
#include "VoxelVolume.h"
#include "MathHelpers.h"
#include <cmath>

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Voxel::VVoxelVolume> VolumeRaytracer::Voxelizer::VVolumeConverter::ConvertMeshInfoToVoxelVolume(const VMeshInfo& meshInfo)
{
	float extends = VMathHelpers::Max(meshInfo.Bounds.GetExtends().X, VMathHelpers::Max(meshInfo.Bounds.GetExtends().Y, meshInfo.Bounds.GetExtends().Z));

	VObjectPtr<Voxel::VVoxelVolume> volume = VObject::CreateObject<Voxel::VVoxelVolume>(3, extends);

	for (size_t index = 0; index < (meshInfo.Indices.size() - 3); index += 3)
	{
		const VVertex& v1 = meshInfo.Vertices[meshInfo.Indices[index]];
		const VVertex& v2 = meshInfo.Vertices[meshInfo.Indices[index + 1]];
		const VVertex& v3 = meshInfo.Vertices[meshInfo.Indices[index + 2]];

		VoxelizeTriangle(volume, v1, v2, v3);
	}

	return volume;
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::VoxelizeTriangle(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v1, const VVertex& v2, const VVertex& v3)
{
	VTriangle triangle;

	triangle.Mid = GetTriangleMidpoint(v1, v2, v3);
	triangle.Normal = GetTriangleNormal(v1, v2, v3);
	triangle.V1 = v1.Position;
	triangle.V2 = v2.Position;
	triangle.V3 = v3.Position;

	VVertexCellIndex cornerIndices[3];

	cornerIndices[0] = GetCellIndex(volume, v1);
	cornerIndices[1] = GetCellIndex(volume, v2);
	cornerIndices[2] = GetCellIndex(volume, v3);

	VVertexCellIndex minCellIndex;
	VVertexCellIndex maxCellIndex;

	minCellIndex.X = VMathHelpers::Min(cornerIndices[0].X, VMathHelpers::Min(cornerIndices[1].X, cornerIndices[2].X));
	minCellIndex.Y = VMathHelpers::Min(cornerIndices[0].Y, VMathHelpers::Min(cornerIndices[1].Y, cornerIndices[2].Y));
	minCellIndex.Z = VMathHelpers::Min(cornerIndices[0].Z, VMathHelpers::Min(cornerIndices[1].Z, cornerIndices[2].Z));

	maxCellIndex.X = VMathHelpers::Max(cornerIndices[0].X, VMathHelpers::Max(cornerIndices[1].X, cornerIndices[2].X));
	maxCellIndex.Y = VMathHelpers::Max(cornerIndices[0].Y, VMathHelpers::Max(cornerIndices[1].Y, cornerIndices[2].Y));
	maxCellIndex.Z = VMathHelpers::Max(cornerIndices[0].Z, VMathHelpers::Max(cornerIndices[1].Z, cornerIndices[2].Z));

	for (int x = minCellIndex.X; x <= maxCellIndex.X; x++)
	{
		for (int y = minCellIndex.Y; y <= maxCellIndex.Y; y++)
		{
			for (int z = minCellIndex.Z; z <= maxCellIndex.Z; z++)
			{
				UpdateCellDensity(volume, VVertexCellIndex(x, y, z), triangle);
			}
		}
	}
}

VolumeRaytracer::Voxelizer::VVertexCellIndex VolumeRaytracer::Voxelizer::VVolumeConverter::GetCellIndex(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v)
{
	VVertexCellIndex res;

	volume->RelativePositionToVoxelIndex(v.Position, res.X, res.Y, res.Z);

	int voxelAxisCount = (int)volume->GetSize();

	res.X = VMathHelpers::Clamp(res.X, 0, voxelAxisCount - 2);
	res.Y = VMathHelpers::Clamp(res.Y, 0, voxelAxisCount - 2);
	res.Z = VMathHelpers::Clamp(res.Z, 0, voxelAxisCount - 2);

	return res;
}

VolumeRaytracer::VVector VolumeRaytracer::Voxelizer::VVolumeConverter::GetTriangleNormal(const VVertex& v1, const VVertex& v2, const VVertex& v3)
{
	return VVector::Cross(v2.Position - v1.Position, v3.Position - v1.Position).GetNormalized();
}

VolumeRaytracer::VVector VolumeRaytracer::Voxelizer::VVolumeConverter::GetTriangleMidpoint(const VVertex& v1, const VVertex& v2, const VVertex& v3)
{
	return (v1.Position + v2.Position + v3.Position) / 3;
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::UpdateCellDensity(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertexCellIndex& cellIndex, const VTriangle& triangle)
{
	float voxelDots[8];
	VVertexCellIndex voxelIndices[8];
	VVector voxelPos[8];
	VVector voxelNormals[8];

	voxelIndices[0] = VVertexCellIndex(cellIndex.X, cellIndex.Y, cellIndex.Z);
	voxelIndices[1] = VVertexCellIndex(cellIndex.X, cellIndex.Y + 1, cellIndex.Z);
	voxelIndices[2] = VVertexCellIndex(cellIndex.X + 1, cellIndex.Y + 1, cellIndex.Z);
	voxelIndices[3] = VVertexCellIndex(cellIndex.X + 1, cellIndex.Y, cellIndex.Z);
	voxelIndices[4] = VVertexCellIndex(cellIndex.X, cellIndex.Y, cellIndex.Z + 1);
	voxelIndices[5] = VVertexCellIndex(cellIndex.X, cellIndex.Y + 1, cellIndex.Z + 1);
	voxelIndices[6] = VVertexCellIndex(cellIndex.X + 1, cellIndex.Y + 1, cellIndex.Z + 1);
	voxelIndices[7] = VVertexCellIndex(cellIndex.X + 1, cellIndex.Y, cellIndex.Z + 1);

	voxelPos[0] = volume->VoxelIndexToRelativePosition(voxelIndices[0].X, voxelIndices[0].Y, voxelIndices[0].Z) - triangle.Mid;
	voxelPos[1] = volume->VoxelIndexToRelativePosition(voxelIndices[1].X, voxelIndices[1].Y, voxelIndices[1].Z) - triangle.Mid;
	voxelPos[2] = volume->VoxelIndexToRelativePosition(voxelIndices[2].X, voxelIndices[2].Y, voxelIndices[2].Z) - triangle.Mid;
	voxelPos[3] = volume->VoxelIndexToRelativePosition(voxelIndices[3].X, voxelIndices[3].Y, voxelIndices[3].Z) - triangle.Mid;
	voxelPos[4] = volume->VoxelIndexToRelativePosition(voxelIndices[4].X, voxelIndices[4].Y, voxelIndices[4].Z) - triangle.Mid;
	voxelPos[5] = volume->VoxelIndexToRelativePosition(voxelIndices[5].X, voxelIndices[5].Y, voxelIndices[5].Z) - triangle.Mid;
	voxelPos[6] = volume->VoxelIndexToRelativePosition(voxelIndices[6].X, voxelIndices[6].Y, voxelIndices[6].Z) - triangle.Mid;
	voxelPos[7] = volume->VoxelIndexToRelativePosition(voxelIndices[7].X, voxelIndices[7].Y, voxelIndices[7].Z) - triangle.Mid;

	voxelNormals[0] = VVector::VectorProjection(voxelPos[0], triangle.Normal);
	voxelNormals[1] = VVector::VectorProjection(voxelPos[1], triangle.Normal);
	voxelNormals[2] = VVector::VectorProjection(voxelPos[2], triangle.Normal);
	voxelNormals[3] = VVector::VectorProjection(voxelPos[3], triangle.Normal);
	voxelNormals[4] = VVector::VectorProjection(voxelPos[4], triangle.Normal);
	voxelNormals[5] = VVector::VectorProjection(voxelPos[5], triangle.Normal);
	voxelNormals[6] = VVector::VectorProjection(voxelPos[6], triangle.Normal);
	voxelNormals[7] = VVector::VectorProjection(voxelPos[7], triangle.Normal);

	voxelDots[0] = voxelNormals[0].Dot(triangle.Normal);
	voxelDots[1] = voxelNormals[1].Dot(triangle.Normal);
	voxelDots[2] = voxelNormals[2].Dot(triangle.Normal);
	voxelDots[3] = voxelNormals[3].Dot(triangle.Normal);
	voxelDots[4] = voxelNormals[4].Dot(triangle.Normal);
	voxelDots[5] = voxelNormals[5].Dot(triangle.Normal);
	voxelDots[6] = voxelNormals[6].Dot(triangle.Normal);
	voxelDots[7] = voxelNormals[7].Dot(triangle.Normal);

	if (!(std::signbit(voxelDots[0]) && std::signbit(voxelDots[1]) && std::signbit(voxelDots[2]) && std::signbit(voxelDots[3]) && std::signbit(voxelDots[4]) && std::signbit(voxelDots[5]) && std::signbit(voxelDots[6]) && std::signbit(voxelDots[7])))
	{
		for (int i = 0; i < 8; i++)
		{
			//if (IsPointOnTriangleIfProjected(voxelPos[i], triangle))
			{
				Voxel::VVoxel voxel = volume->GetVoxel(voxelIndices[i].X, voxelIndices[i].Y, voxelIndices[i].Z);

				voxel.Material = 1;

				float density = std::signbit(voxelDots[i]) ? -voxelNormals[i].Length() : voxelNormals[i].Length();

				if (std::abs(voxel.Density) > std::abs(density))
				{
					voxel.Density = density;
				}

				volume->SetVoxel(voxelIndices[i].X, voxelIndices[i].Y, voxelIndices[i].Z, voxel);
			}
		}
	}
}

bool VolumeRaytracer::Voxelizer::VVolumeConverter::IsPointOnTriangleIfProjected(const VVector& relToTrianglePoint, const VTriangle& triangle)
{
	VQuat triRotation = VQuat::FromUpVector(triangle.Normal).Inverse();

	VVector v1 = triRotation * (triangle.V1 - triangle.Mid);
	VVector v2 = triRotation * (triangle.V2 - triangle.Mid);
	VVector v3 = triRotation * (triangle.V3 - triangle.Mid);

	VVector p = triRotation * relToTrianglePoint;
	
	auto& signFunc = [](const VVector& p1, const VVector& p2, const VVector& p3)
	{
		return (p1.X - p3.X) * (p2.Y - p3.Y) - (p2.X - p3.X) * (p1.Y - p3.Y);
	}; 

	float d1, d2, d3;
	bool neg, pos;

	d1 = signFunc(p, v1, v2);
	d2 = signFunc(p, v2, v3);
	d3 = signFunc(p, v3, v1);

	neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(neg && pos);
}
