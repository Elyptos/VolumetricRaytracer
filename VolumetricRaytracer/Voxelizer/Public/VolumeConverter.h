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
#include "AABB.h"

namespace VolumeRaytracer
{
	namespace Voxel
	{
		class VVoxelVolume;
	}

	namespace Voxelizer
	{
		struct VEdgeRay
		{
			VVector Origin;
			VVector Direction;
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

		struct VTriangleRegions
		{
		public:
			VVector ANorm;
			float BLength;
			VVector BNorm;
			float CLength;
			VVector CNorm;
			float DLength;
			VVector DNorm;
			VVector ENorm;
			VVector FNorm;
			VVector GNorm;
		};

		struct VTriangleRegionalVoxelDistances
		{
		public:
			float A;
			float B;
			float C;
			float D;
			float E;
			float F;
			float G;
		};

		enum class EVTriangleRegion
		{
			R1,
			R2,
			R3,
			R4,
			R5,
			R6,
			R7
		};

		class VVolumeConverter
		{
		public:
			static VObjectPtr<Voxel::VVoxelVolume> ConvertMeshInfoToVoxelVolume(const VMeshInfo& meshInfo, const VTextureLibrary& textureLib);

		private:
			static void VoxelizeVertex(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v);
			static void VoxelizeEdge(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v1, const VVertex& v2);
			static void VoxelizeFace(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v1, const VVertex& v2, const VVertex& v3, const float& surfaceThreshold);

			static void VoxelizeTriangle(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v1, const VVertex& v2, const VVertex& v3, const float& surfaceThreshold);

			static VIntVector GetCellIndex(std::shared_ptr<Voxel::VVoxelVolume> volume, const VVertex& v);

			static VVector GetTriangleNormal(const VVertex& v1, const VVertex& v2, const VVertex& v3);
			static VVector GetTriangleMidpoint(const VVertex& v1, const VVertex& v2, const VVertex& v3);

			static float GetNearestDistanceFromEdgeToPoint(const VVector& point, /*const VVector& cellStart, const VVector& cellEnd, */const VVector& edgeStart, const VVector& edgeEnd);
			static float GetNearestDinstanceFromPlaneToPoint(const VVector& point, const VVector& planeNormal);

			static bool IsPointOnTriangleIfProjected(const VVector& relToTrianglePoint, const VTriangle& triangle);
			static bool IsPointInTriangle(const VVector2D& point, const VVector2D& v1, const VVector2D& v2, const VVector2D& v3);

			static bool IsTriangleInsideVoxelBounds(std::shared_ptr<Voxel::VVoxelVolume> volume, const VIntVector& voxelIndex, const VTriangle& triangle);

			static VIntVector GoToNextVoxelAlongEdge(std::shared_ptr<Voxel::VVoxelVolume> volume, const VEdgeRay& ray, const VIntVector& voxelIndex, const VIntVector& voxelDir, float& outNewT);

			static void UpdateCellDensityWithEdgeIntersection(std::shared_ptr<Voxel::VVoxelVolume> volume, const VIntVector& voxelIndex, const VVector& edgeStart, const VVector& edgeEnd);
			static void UpdateCellDensityWithTriangleIntersection(std::shared_ptr<Voxel::VVoxelVolume> volume, const VIntVector& voxelIndex, const VTriangle& triangle);

			static bool ExtractResolutionFromName(const std::string& name, uint8_t& outResolution);

			static VAABB GetTriangleBoundingBox(const VTriangle& triangle, const float& threshold);
			static void GetVoxelizedBoundingBox(std::shared_ptr<Voxel::VVoxelVolume> volume, const VAABB& aabb, VIntVector& outMin, VIntVector& outMax, const float& threshold);

			static VTriangleRegions CalculateTriangleRegionVectors(const VTriangle& triangle);
			static VTriangleRegionalVoxelDistances CalculateTriangleRegionDistances(const VTriangleRegions& regions, const VTriangle& triangle, const VVector& point);

			static EVTriangleRegion GetTriangleRegion(const VTriangleRegions& regions, const VTriangleRegionalVoxelDistances& distances);
		};
	}
}