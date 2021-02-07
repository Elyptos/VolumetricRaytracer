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
#include <iostream>
#include <cmath>

namespace VolumeRaytracer
{
	namespace VolumeConversionInternal
	{
		const float W = 2 * std::sqrt(3);
		//const float W = 1.f;
	}
}

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Voxel::VVoxelVolume> VolumeRaytracer::Voxelizer::VVolumeConverter::ConvertMeshInfoToVoxelVolume(const VMeshInfo& meshInfo)
{
	float extends = VMathHelpers::Max(meshInfo.Bounds.GetExtends().X, VMathHelpers::Max(meshInfo.Bounds.GetExtends().Y, meshInfo.Bounds.GetExtends().Z));
	extends += extends * 0.25f;

	uint8_t desiredResolution = 5;

	if (!ExtractResolutionFromName(meshInfo.MeshName, desiredResolution))
	{
		desiredResolution = 5;
		std::cout << "[WARNING] Mesh with name " << meshInfo.MeshName << " has no or invalid resolution specifier. Correct syntax is meshName_resolution (cubeMesh_6). Using default resolution of 5!" << std::endl;
	}

	VObjectPtr<Voxel::VVoxelVolume> volume = VObject::CreateObject<Voxel::VVoxelVolume>(desiredResolution, extends);

	Voxel::VVoxel defaultVoxel;
	defaultVoxel.Material = 0;
	defaultVoxel.Density = extends * 2.f;

	volume->FillVolume(defaultVoxel);

	float extractionThreshold = volume->GetCellSize() /** 0.5f*/ * std::sqrt(3);

	for (size_t index = 0; index <= (meshInfo.Indices.size() - 3); index += 3)
	{
		const VVertex& v1 = meshInfo.Vertices[meshInfo.Indices[index]];
		const VVertex& v2 = meshInfo.Vertices[meshInfo.Indices[index + 1]];
		const VVertex& v3 = meshInfo.Vertices[meshInfo.Indices[index + 2]];

		VoxelizeTriangle(volume, v1, v2, v3, extractionThreshold);
	}

	volume->SetMaterial(meshInfo.Material);

	return volume;
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::VoxelizeVertex(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v)
{
	VIntVector index;

	index = volume->RelativePositionToVoxelIndex(v.Position);

	if (volume->IsValidVoxelIndex(index))
	{
		Voxel::VVoxel v = volume->GetVoxel(index);

		if (v.Density > 0)
		{
			v.Density = -1;
			v.Material = 1;

			volume->SetVoxel(index, v);
		}
	}
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::VoxelizeEdge(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v1, const VVertex& v2)
{
	VEdgeRay ray;

	ray.Origin = v1.Position;
	ray.Direction = (v2.Position - v1.Position).GetNormalized();

	float t = 0.f;
	float tExit = 0.f;
	float tMax = (v2.Position - v1.Position).Length();

	VIntVector voxelTravelingDir;
	voxelTravelingDir.X = VMathHelpers::Sign(ray.Direction.X);
	voxelTravelingDir.Y = VMathHelpers::Sign(ray.Direction.Y);
	voxelTravelingDir.Z = VMathHelpers::Sign(ray.Direction.Z);

	VIntVector voxelIndex = volume->RelativePositionToVoxelIndex(v1.Position);;
	VIntVector nextVoxel;

	auto& getPositionAlongRay = [](const VEdgeRay& ray, const float& t)
	{
		return ray.Origin + ray.Direction * t;
	};

	while (t <= tMax)
	{
		nextVoxel = GoToNextVoxelAlongEdge(volume, ray, voxelIndex, voxelTravelingDir, tExit);

		if (volume->IsValidVoxelIndex(voxelIndex))
		{
			Voxel::VVoxel v = volume->GetVoxel(voxelIndex);

			float density = -GetNearestDistanceFromEdgeToPoint(volume->VoxelIndexToRelativePosition(voxelIndex), /*getPositionAlongRay(ray, t), getPositionAlongRay(ray, tExit),*/ v1.Position, v2.Position);

			if (v.Density > 0)
			{
				v.Density = density;
			}
			else
			{
				v.Density = VMathHelpers::Max(v.Density, density);
			}

			v.Material = 1;

			volume->SetVoxel(voxelIndex, v);
		}

		//UpdateCellDensityWithEdgeIntersection(volume, voxelIndex, v1.Position, v2.Position);

		voxelIndex = nextVoxel;
		t = tExit;
	}
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::VoxelizeFace(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v1, const VVertex& v2, const VVertex& v3, const float& surfaceThreshold)
{
	VTriangle triangle;
	triangle.V1 = v1.Position;
	triangle.V2 = v2.Position;
	triangle.V3 = v3.Position;
	triangle.Mid = GetTriangleMidpoint(v1, v2, v3);
	triangle.Normal = GetTriangleNormal(v1, v2, v3);

	VTriangleRegions triangleRegions = CalculateTriangleRegionVectors(triangle);

	VAABB triangleBoundingBox = GetTriangleBoundingBox(triangle, 0.f);

	VIntVector minCellIndex;
	VIntVector maxCellIndex;

	GetVoxelizedBoundingBox(volume, triangleBoundingBox, minCellIndex, maxCellIndex, surfaceThreshold);
	
	minCellIndex = minCellIndex.Max(minCellIndex, 0);
	maxCellIndex = maxCellIndex.Min(maxCellIndex, volume->GetSize() - 1);

	for (int x = minCellIndex.X; x <= maxCellIndex.X; x++)
	{
		for (int y = minCellIndex.Y; y <= maxCellIndex.Y; y++)
		{
			for (int z = minCellIndex.Z; z <= maxCellIndex.Z; z++)
			{
				VIntVector voxelIndex = VIntVector(x, y, z);
				VVector voxelPos = volume->VoxelIndexToRelativePosition(voxelIndex);

				VTriangleRegionalVoxelDistances distances = CalculateTriangleRegionDistances(triangleRegions, triangle, voxelPos);

				EVTriangleRegion region = GetTriangleRegion(triangleRegions, distances);

				Voxel::VVoxel voxel = volume->GetVoxel(voxelIndex);
				float density = voxel.Density;

				switch (region)
				{
				case EVTriangleRegion::R1:
					density = 1.f - (std::abs(distances.A) / surfaceThreshold);
					density = -1.f * density + 0.5f;
					break;
				case EVTriangleRegion::R2:
					density = 1.f - (std::sqrt(distances.A * distances.A + distances.G * distances.G) / surfaceThreshold);
					density = -1.f * density + 0.5f;
					break;
				case EVTriangleRegion::R3:
					density = 1.f - (std::sqrt(distances.A * distances.A + distances.F * distances.F) / surfaceThreshold);
					density = -1.f * density + 0.5f;
					break;
				case EVTriangleRegion::R4:
					density = 1.f - (std::sqrt(distances.A * distances.A + distances.E * distances.E) / surfaceThreshold);
					density = -1.f * density + 0.5f;
					break;
				case EVTriangleRegion::R5:
					density = 1.f - ((voxelPos - triangle.V1).Length() / surfaceThreshold);
					density = -1.f * density + 0.5f;
					break;
				case EVTriangleRegion::R6:
					density = 1.f - ((voxelPos - triangle.V2).Length() / surfaceThreshold);
					density = -1.f * density + 0.5f;
					break;
				case EVTriangleRegion::R7:
					density = 1.f - ((voxelPos - triangle.V3).Length() / surfaceThreshold);
					density = -1.f * density + 0.5f;
					break;
				}


				/*if (std::abs(density) < std::abs(voxel.Density))
				{
					if (!(voxel.Density < 0.f && density > 0.f))
					{
						voxel.Density = density;
						voxel.Material = voxel.Density <= 0.f ? 1 : 0;

						volume->SetVoxel(voxelIndex, voxel);
					}
				}*/

				if (density < voxel.Density)
				{
					voxel.Density = density;
					voxel.Material = voxel.Density <= 0.f ? 1 : 0;

					volume->SetVoxel(voxelIndex, voxel);
				}
			}
		}
	}
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::VoxelizeTriangle(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v1, const VVertex& v2, const VVertex& v3, const float& surfaceThreshold)
{
	//VoxelizeEdge(volume, v1, v2);
	//VoxelizeEdge(volume, v1, v3);
	//VoxelizeEdge(volume, v2, v3);

	VoxelizeFace(volume, v1, v2, v3, surfaceThreshold);
}

VolumeRaytracer::VIntVector VolumeRaytracer::Voxelizer::VVolumeConverter::GetCellIndex(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v)
{
	VIntVector res = volume->RelativePositionToCellIndex(v.Position);

	int voxelAxisCount = (int)volume->GetSize();

	res.X = VMathHelpers::Clamp(res.X, 0, voxelAxisCount - 2);
	res.Y = VMathHelpers::Clamp(res.Y, 0, voxelAxisCount - 2);
	res.Z = VMathHelpers::Clamp(res.Z, 0, voxelAxisCount - 2);

	return res;
}

VolumeRaytracer::VVector VolumeRaytracer::Voxelizer::VVolumeConverter::GetTriangleNormal(const VVertex& v1, const VVertex& v2, const VVertex& v3)
{
	return VVector::Cross(v2.Position - v1.Position, v3.Position - v1.Position).GetNormalized();

	//VVector normal = VVector::Cross(v2.Position - v1.Position, v3.Position - v1.Position).GetNormalized();
	//VVector sumVertexNormal = v1.Normal + v2.Normal + v3.Normal;

	//if (v1.Normal.X < 0)
	//{
	//	int i = 0;
	//}

	//bool flipNormal = normal.Dot(sumVertexNormal) < 0;

	//return flipNormal ? -normal : normal;
}

VolumeRaytracer::VVector VolumeRaytracer::Voxelizer::VVolumeConverter::GetTriangleMidpoint(const VVertex& v1, const VVertex& v2, const VVertex& v3)
{
	return (v1.Position + v2.Position + v3.Position) / 3;
}

float VolumeRaytracer::Voxelizer::VVolumeConverter::GetNearestDistanceFromEdgeToPoint(const VVector& point, /*const VVector& cellStart, const VVector& cellEnd, */const VVector& edgeStart, const VVector& edgeEnd)
{
	VVector d = (edgeEnd - edgeStart).GetNormalized();
	VVector o = edgeStart;

	double a = (double)d.X * (double)d.X + (double)d.Y * (double)d.Y + (double)d.Z * (double)d.Z;
	double b = 2. * d.X * ((double)o.X - point.X) + 2. * d.Y * ((double)o.Y - point.Y) + 2. * d.Z * ((double)o.Z - point.Z);
	double c = std::pow((double)point.X - o.X, 2) + std::pow(point.Y - o.Y, 2) + std::pow(point.Z - o.Z, 2);

	double t = -b / (2*a);

	double tEnd = (edgeEnd - edgeStart).Length();

	t = VMathHelpers::Clamp(t, 0., tEnd);

	return std::sqrt(a * t * t + b * t + c);
}

float VolumeRaytracer::Voxelizer::VVolumeConverter::GetNearestDinstanceFromPlaneToPoint(const VVector& point, const VVector& planeNormal)
{
	return VVector::VectorProjection(point, planeNormal).Length();
}

//void VolumeRaytracer::Voxelizer::VVolumeConverter::UpdateCellDensity(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVoxelIndex& cellIndex, const VTriangle& triangle)
//{
//	float voxelDots[8];
//	VVoxelIndex voxelIndices[8];
//	VVector voxelPos[8];
//	VVector voxelNormals[8];
//
//	voxelIndices[0] = VVoxelIndex(cellIndex.X, cellIndex.Y, cellIndex.Z);
//	voxelIndices[1] = VVoxelIndex(cellIndex.X, cellIndex.Y + 1, cellIndex.Z);
//	voxelIndices[2] = VVoxelIndex(cellIndex.X + 1, cellIndex.Y + 1, cellIndex.Z);
//	voxelIndices[3] = VVoxelIndex(cellIndex.X + 1, cellIndex.Y, cellIndex.Z);
//	voxelIndices[4] = VVoxelIndex(cellIndex.X, cellIndex.Y, cellIndex.Z + 1);
//	voxelIndices[5] = VVoxelIndex(cellIndex.X, cellIndex.Y + 1, cellIndex.Z + 1);
//	voxelIndices[6] = VVoxelIndex(cellIndex.X + 1, cellIndex.Y + 1, cellIndex.Z + 1);
//	voxelIndices[7] = VVoxelIndex(cellIndex.X + 1, cellIndex.Y, cellIndex.Z + 1);
//
//	voxelPos[0] = volume->VoxelIndexToRelativePosition(voxelIndices[0].X, voxelIndices[0].Y, voxelIndices[0].Z) - triangle.Mid;
//	voxelPos[1] = volume->VoxelIndexToRelativePosition(voxelIndices[1].X, voxelIndices[1].Y, voxelIndices[1].Z) - triangle.Mid;
//	voxelPos[2] = volume->VoxelIndexToRelativePosition(voxelIndices[2].X, voxelIndices[2].Y, voxelIndices[2].Z) - triangle.Mid;
//	voxelPos[3] = volume->VoxelIndexToRelativePosition(voxelIndices[3].X, voxelIndices[3].Y, voxelIndices[3].Z) - triangle.Mid;
//	voxelPos[4] = volume->VoxelIndexToRelativePosition(voxelIndices[4].X, voxelIndices[4].Y, voxelIndices[4].Z) - triangle.Mid;
//	voxelPos[5] = volume->VoxelIndexToRelativePosition(voxelIndices[5].X, voxelIndices[5].Y, voxelIndices[5].Z) - triangle.Mid;
//	voxelPos[6] = volume->VoxelIndexToRelativePosition(voxelIndices[6].X, voxelIndices[6].Y, voxelIndices[6].Z) - triangle.Mid;
//	voxelPos[7] = volume->VoxelIndexToRelativePosition(voxelIndices[7].X, voxelIndices[7].Y, voxelIndices[7].Z) - triangle.Mid;
//
//	voxelNormals[0] = VVector::VectorProjection(voxelPos[0], triangle.Normal);
//	voxelNormals[1] = VVector::VectorProjection(voxelPos[1], triangle.Normal);
//	voxelNormals[2] = VVector::VectorProjection(voxelPos[2], triangle.Normal);
//	voxelNormals[3] = VVector::VectorProjection(voxelPos[3], triangle.Normal);
//	voxelNormals[4] = VVector::VectorProjection(voxelPos[4], triangle.Normal);
//	voxelNormals[5] = VVector::VectorProjection(voxelPos[5], triangle.Normal);
//	voxelNormals[6] = VVector::VectorProjection(voxelPos[6], triangle.Normal);
//	voxelNormals[7] = VVector::VectorProjection(voxelPos[7], triangle.Normal);
//
//	voxelDots[0] = voxelNormals[0].Dot(triangle.Normal);
//	voxelDots[1] = voxelNormals[1].Dot(triangle.Normal);
//	voxelDots[2] = voxelNormals[2].Dot(triangle.Normal);
//	voxelDots[3] = voxelNormals[3].Dot(triangle.Normal);
//	voxelDots[4] = voxelNormals[4].Dot(triangle.Normal);
//	voxelDots[5] = voxelNormals[5].Dot(triangle.Normal);
//	voxelDots[6] = voxelNormals[6].Dot(triangle.Normal);
//	voxelDots[7] = voxelNormals[7].Dot(triangle.Normal);
//
//	if (!(std::signbit(voxelDots[0]) && std::signbit(voxelDots[1]) && std::signbit(voxelDots[2]) && std::signbit(voxelDots[3]) && std::signbit(voxelDots[4]) && std::signbit(voxelDots[5]) && std::signbit(voxelDots[6]) && std::signbit(voxelDots[7])))
//	{
//		for (int i = 0; i < 8; i++)
//		{
//			//if (IsPointOnTriangleIfProjected(voxelPos[i], triangle))
//			{
//				Voxel::VVoxel voxel = volume->GetVoxel(voxelIndices[i].X, voxelIndices[i].Y, voxelIndices[i].Z);
//
//				voxel.Material = 1;
//
//				float density = std::signbit(voxelDots[i]) ? -voxelNormals[i].Length() : voxelNormals[i].Length();
//
//				if (std::abs(voxel.Density) > std::abs(density))
//				{
//					voxel.Density = density;
//				}
//
//				volume->SetVoxel(voxelIndices[i].X, voxelIndices[i].Y, voxelIndices[i].Z, voxel);
//			}
//		}
//	}
//}

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

bool VolumeRaytracer::Voxelizer::VVolumeConverter::IsPointInTriangle(const VVector2D& point, const VVector2D& v1, const VVector2D& v2, const VVector2D& v3)
{
	auto& signFunc = [](const VVector2D& p1, const VVector2D& p2, const VVector2D& p3)
	{
		return (p1.X - p3.X) * (p2.Y - p3.Y) - (p2.X - p3.X) * (p1.Y - p3.Y);
	};

	float d1, d2, d3;
	bool neg, pos;

	d1 = signFunc(point, v1, v2);
	d2 = signFunc(point, v2, v3);
	d3 = signFunc(point, v3, v1);

	neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(neg && pos);
}

bool VolumeRaytracer::Voxelizer::VVolumeConverter::IsTriangleInsideVoxelBounds(std::shared_ptr<Voxel::VVoxelVolume> volume, const VIntVector& voxelIndex, const VTriangle& triangle)
{
	const VVector voxelCornerOffsets[8] = 
	{
		VVector(-1, -1, -1),
		VVector(-1, 1, -1),
		VVector(1, 1, -1),
		VVector(1, -1, -1),
		VVector(-1, -1, 1),
		VVector(-1, 1, 1),
		VVector(1, 1, 1),
		VVector(1, -1, 1)
	};

	bool hasPos = false;
	bool hasNeg = false;

	VVector voxelPos = volume->VoxelIndexToRelativePosition(voxelIndex);

	for (int i = 0; i < 8; i++)
	{
		VVector relCornerPos = voxelPos + (voxelCornerOffsets[i] * volume->GetCellSize() * 0.5f) - triangle.Mid;
		VVector normal = VVector::VectorProjection(relCornerPos, triangle.Normal);

		float dot = normal.Dot(triangle.Normal);

		if (dot <= 0)
		{
			hasNeg = true;
		}
		else
		{
			hasPos = true;
		}
	}

	return hasPos && hasNeg;
}

VolumeRaytracer::VIntVector VolumeRaytracer::Voxelizer::VVolumeConverter::GoToNextVoxelAlongEdge(std::shared_ptr<Voxel::VVoxelVolume> volume, const VEdgeRay& ray, const VIntVector& voxelIndex, const VIntVector& voxelDir, float& outNewT)
{
	VVector tMax = VVector(100000, 100000, 100000);
	VIntVector res = voxelIndex;
	constexpr float FLT_INFINITY = std::numeric_limits<float>::infinity();
	VVector invRayDirection;
	
	invRayDirection.X = ray.Direction.X != 0 ? 1 / ray.Direction.X : (ray.Direction.X > 0) ? FLT_INFINITY : -FLT_INFINITY;
	invRayDirection.Y = ray.Direction.Y != 0 ? 1 / ray.Direction.Y : (ray.Direction.Y > 0) ? FLT_INFINITY : -FLT_INFINITY;
	invRayDirection.Z = ray.Direction.Z != 0 ? 1 / ray.Direction.Z : (ray.Direction.Z > 0) ? FLT_INFINITY : -FLT_INFINITY;

	int signX = ray.Direction.X > 0;
	int signY = ray.Direction.Y > 0;
	int signZ = ray.Direction.Z > 0;

	VVector voxelPos = volume->VoxelIndexToRelativePosition(voxelIndex);
	VVector voxelExtends = VVector::ONE * volume->GetCellSize() * 0.5;
	VVector aabb[2] = { voxelPos - voxelExtends, voxelPos + voxelExtends };

	if (ray.Direction.X != 0)
		tMax.X = (aabb[signX].X - ray.Origin.X) * invRayDirection.X;

	if (ray.Direction.Y != 0)
		tMax.Y = (aabb[signY].Y - ray.Origin.Y) * invRayDirection.Y;

	if (ray.Direction.Z != 0)
		tMax.Z = (aabb[signZ].Z - ray.Origin.Z) * invRayDirection.Z;

	if (tMax.X < tMax.Y)
	{
		if (tMax.X < tMax.Z)
		{
			res.X += voxelDir.X;
			outNewT = tMax.X;
		}
		else
		{
			res.Z += voxelDir.Z;
			outNewT = tMax.Z;
		}
	}
	else
	{
		if (tMax.Y < tMax.Z)
		{
			res.Y += voxelDir.Y;
			outNewT = tMax.Y;
		}
		else
		{
			res.Z += voxelDir.Z;
			outNewT = tMax.Z;
		}
	}

	return res;
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::UpdateCellDensityWithEdgeIntersection(std::shared_ptr<Voxel::VVoxelVolume> volume, const VIntVector& voxelIndex, const VVector& edgeStart, const VVector& edgeEnd)
{
	const VIntVector cellOffset[8] = {
		VIntVector(0,0,0),
		VIntVector(-1,0,0),
		VIntVector(0,-1,0),
		VIntVector(0,0,-1),
		VIntVector(-1,-1,0),
		VIntVector(0,-1,-1),
		VIntVector(-1,-1,-1),
	};

	const VIntVector cellCornerOffsets[8] =
	{
		VIntVector(0, 0, 0),
		VIntVector(1, 0, 0),
		VIntVector(0, 1, 0),
		VIntVector(1, 1, 0),
		VIntVector(0, 0, 1),
		VIntVector(1, 0, 1),
		VIntVector(0, 1, 1),
		VIntVector(1, 1, 1)
	};

	for (int cell = 0; cell < 8; cell++)
	{
		for (int v = 0; v < 8; v++)
		{
			VIntVector cornerVoxelIndex = (voxelIndex - cellOffset[cell]) + cellCornerOffsets[v];

			if (volume->IsValidVoxelIndex(cornerVoxelIndex))
			{
				Voxel::VVoxel voxel = volume->GetVoxel(cornerVoxelIndex);

				float density = GetNearestDistanceFromEdgeToPoint(volume->VoxelIndexToRelativePosition(cornerVoxelIndex), edgeStart, edgeEnd);

				if (cornerVoxelIndex == voxelIndex)
				{
					if (voxel.Density > 0)
					{
						voxel.Density = -density;
					}
					else
					{
						voxel.Density = VMathHelpers::Max(voxel.Density, -density);
					}
				}
				else
				{
					if(voxel.Density > 0)
						voxel.Density = VMathHelpers::Min(voxel.Density, density);
				}

				//v.Material = 1;

				volume->SetVoxel(cornerVoxelIndex, voxel);
			}
		}
	}
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::UpdateCellDensityWithTriangleIntersection(std::shared_ptr<Voxel::VVoxelVolume> volume, const VIntVector& voxelIndex, const VTriangle& triangle)
{
	const VIntVector cellOffset[8] = {
			VIntVector(0,0,0),
			VIntVector(-1,0,0),
			VIntVector(0,-1,0),
			VIntVector(0,0,-1),
			VIntVector(-1,-1,0),
			VIntVector(0,-1,-1),
			VIntVector(-1,-1,-1),
	};

	const VIntVector cellCornerOffsets[8] =
	{
		VIntVector(0, 0, 0),
		VIntVector(1, 0, 0),
		VIntVector(0, 1, 0),
		VIntVector(1, 1, 0),
		VIntVector(0, 0, 1),
		VIntVector(1, 0, 1),
		VIntVector(0, 1, 1),
		VIntVector(1, 1, 1)
	};

	for (int cell = 0; cell < 8; cell++)
	{
		for (int v = 0; v < 8; v++)
		{
			VIntVector cornerVoxelIndex = (voxelIndex - cellOffset[cell]) + cellCornerOffsets[v];

			if (volume->IsValidVoxelIndex(cornerVoxelIndex))
			{
				VVector relVoxelPos = volume->VoxelIndexToRelativePosition(cornerVoxelIndex) - triangle.Mid;
				bool bInsideSurface = triangle.Normal.Dot(relVoxelPos) < 0.f;
				float density = GetNearestDinstanceFromPlaneToPoint(relVoxelPos, triangle.Normal);

				density = bInsideSurface ? -density : density;

				Voxel::VVoxel voxel = volume->GetVoxel(cornerVoxelIndex);

				if (cornerVoxelIndex == voxelIndex)
				{
					if (voxel.Density > 0)
					{
						voxel.Density = density;
					}
					else
					{
						voxel.Density = VMathHelpers::Max(voxel.Density, density);
					}
				}
				else
				{
					if (voxel.Density > 0)
						voxel.Density = VMathHelpers::Min(voxel.Density, density);
				}

				volume->SetVoxel(cornerVoxelIndex, voxel);
			}
		}
	}
}

bool VolumeRaytracer::Voxelizer::VVolumeConverter::ExtractResolutionFromName(const std::string& name, uint8_t& outResolution)
{
	size_t terminatorIndex = name.rfind('_');

	if (terminatorIndex != std::string::npos)
	{
		std::string resStr = name.substr(terminatorIndex + 1, terminatorIndex - name.size());

		int res = 0;

		try
		{
			res = std::stoi(resStr);
			outResolution = (uint8_t)res;
			return true;
		}
		catch (std::invalid_argument)
		{}
		catch (std::out_of_range)
		{}
	}

	return false;
}

VolumeRaytracer::VAABB VolumeRaytracer::Voxelizer::VVolumeConverter::GetTriangleBoundingBox(const VTriangle& triangle, const float& threshold)
{
	VAABB res;

	VVector min = VVector::Min(triangle.V1, VVector::Min(triangle.V2, triangle.V3)) - VVector::ONE * threshold;
	VVector max = VVector::Max(triangle.V1, VVector::Max(triangle.V2, triangle.V3)) + VVector::ONE * threshold;

	res.SetCenterPosition((max - min) * 0.5f + min);
	res.SetExtends((max - min) * 0.5f);

	return res;
}

void VolumeRaytracer::Voxelizer::VVolumeConverter::GetVoxelizedBoundingBox(std::shared_ptr<Voxel::VVoxelVolume> volume, const VAABB& aabb, VIntVector& outMin, VIntVector& outMax, const float& threshold)
{
	VVector min = aabb.GetMin() - VVector::ONE * threshold;
	VVector max = aabb.GetMax() + VVector::ONE * threshold;

	outMin = volume->RelativePositionToVoxelIndex(min) - VIntVector::ONE;
	outMax = volume->RelativePositionToVoxelIndex(max) + VIntVector::ONE;
}

VolumeRaytracer::Voxelizer::VTriangleRegions VolumeRaytracer::Voxelizer::VVolumeConverter::CalculateTriangleRegionVectors(const VTriangle& triangle)
{
	VTriangleRegions res;

	res.ANorm = triangle.Normal;

	res.BNorm = triangle.V3 - triangle.V1;
	res.BLength = res.BNorm.Length();
	res.BNorm.Normalize();

	res.CNorm = triangle.V2 - triangle.V3;
	res.CLength = res.CNorm.Length();
	res.CNorm.Normalize();

	res.DNorm = triangle.V1 - triangle.V2;
	res.DLength = res.DNorm.Length();
	res.DNorm.Normalize();

	res.GNorm = VVector::Cross(res.DNorm, res.ANorm).GetNormalized();
	res.ENorm = VVector::Cross(res.BNorm, res.ANorm).GetNormalized();
	res.FNorm = VVector::Cross(res.CNorm, res.ANorm).GetNormalized();

	return res;
}

VolumeRaytracer::Voxelizer::VTriangleRegionalVoxelDistances VolumeRaytracer::Voxelizer::VVolumeConverter::CalculateTriangleRegionDistances(const VTriangleRegions& regions, const VTriangle& triangle, const VVector& point)
{
	VTriangleRegionalVoxelDistances res;

	VVector relV1 = point - triangle.V1;
	VVector relV2 = point - triangle.V2;
	VVector relV3 = point - triangle.V3;

	res.A = relV1.Dot(regions.ANorm);
	res.B = relV1.Dot(regions.BNorm);
	res.C = relV3.Dot(regions.CNorm);
	res.D = relV2.Dot(regions.DNorm);
	res.E = relV1.Dot(regions.ENorm);
	res.F = relV3.Dot(regions.FNorm);
	res.G = relV2.Dot(regions.GNorm);

	return res;
}

VolumeRaytracer::Voxelizer::EVTriangleRegion VolumeRaytracer::Voxelizer::VVolumeConverter::GetTriangleRegion(const VTriangleRegions& regions, const VTriangleRegionalVoxelDistances& distances)
{
	if (distances.E >= 0.f && distances.F >= 0.f && distances.G >= 0.f)
	{
		return EVTriangleRegion::R1;
	}
	else if (distances.D >= regions.DLength && distances.B <= 0.f)
	{
		return EVTriangleRegion::R5;
	}
	else if (distances.B >= regions.BLength && distances.C <= 0.f)
	{
		return EVTriangleRegion::R7;
	}
	else if (distances.C >= regions.CLength && distances.D <= 0.f)
	{
		return EVTriangleRegion::R6;
	}
	else if (distances.G <= 0.f && distances.D >= 0.f && distances.D <= regions.DLength)
	{
		return EVTriangleRegion::R2;
	}
	else if (distances.E <= 0.f && distances.B >= 0.f && distances.B <= regions.BLength)
	{
		return EVTriangleRegion::R4;
	}
	else if(distances.F <= 0.f && distances.C >= 0.f && distances.C <= regions.CLength)
	{
		return EVTriangleRegion::R3;
	}
	else
	{
		assert(1==2, "This code segment should not be reachable");
	}
}
