/*
	Copyright (c) 2021 Thomas Schöngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

struct VoxelOctreeNode
{
	float size;
	float3 nodePos;
};

int3 WorldSpaceToVoxelSpace(in float3 worldLocation)
{
	uint instance = InstanceID();

	float3 volumeOrigin = -g_geometryCB[instance].volumeExtend;
	float3 relativeVolumePos = worldLocation - volumeOrigin;
	float distanceBetweenVoxel = g_geometryCB[instance].distanceBtwVoxels;

	return int3(floor(relativeVolumePos.x / distanceBetweenVoxel), floor(relativeVolumePos.y / distanceBetweenVoxel), floor(relativeVolumePos.z / distanceBetweenVoxel));
}

float3 VoxelIndexToWorldSpace(in int3 voxelIndex)
{
	uint instance = InstanceID();

	float distanceBetweenVoxel = g_geometryCB[instance].distanceBtwVoxels;
	float3 voxelIndexF = voxelIndex;
	float3 volumeOrigin = -g_geometryCB[instance].volumeExtend;

	return voxelIndexF * distanceBetweenVoxel + volumeOrigin;
}

float3 WorldSpaceToBottomLevelCellSpace(in int3 cellOrigin, in float cellSize, in float3 worldSpace)
{
	uint instance = InstanceID();
	float3 voxelPos = VoxelIndexToWorldSpace(cellOrigin);

	return (worldSpace - voxelPos) / cellSize;
}

bool IsValidCell(in int3 originVoxelPos)
{
	uint instance = InstanceID();

	return  originVoxelPos.x >= 0 &&
			originVoxelPos.y >= 0 &&
			originVoxelPos.z >= 0 &&
			(originVoxelPos.x + 1) < g_geometryCB[instance].voxelAxisCount &&
			(originVoxelPos.y + 1) < g_geometryCB[instance].voxelAxisCount &&
			(originVoxelPos.z + 1) < g_geometryCB[instance].voxelAxisCount;
}

inline float GetNodeSize(in int depth)
{
	uint instanceID = InstanceID();
	int maxDepth = g_geometryCB[instanceID].octreeDepth;
	float distanceBtwVoxels = g_geometryCB[instanceID].distanceBtwVoxels;

	return pow(2, maxDepth - depth) * distanceBtwVoxels;
}

inline int GetChildCountOfNode(in int depth)
{
	uint instanceID = InstanceID();
	int maxDepth = g_geometryCB[instanceID].octreeDepth;

	return pow(2, maxDepth - depth);
}

int3 GoToNextVoxel(in Ray ray, in float3 voxelPos, in float cellSize, out float newT)
{
	float3 tMax = float3(100000, 100000, 100000);
	int3 res = int3(0,0,0);
	const float FLT_INFINITY = 1.#INF;
	float3 invRayDirection = ray.direction != 0 ? 1 / ray.direction : (ray.direction > 0) ? FLT_INFINITY : -FLT_INFINITY;

	int3 sign = ray.direction > 0;

	float3 aabb[2] = { voxelPos, voxelPos + cellSize };

	if(ray.direction.x != 0)
		tMax.x = (aabb[sign.x].x - ray.origin.x) * invRayDirection.x;
	
	if(ray.direction.y != 0)
		tMax.y = (aabb[sign.y].y - ray.origin.y) * invRayDirection.y;
	
	if(ray.direction.z != 0)
		tMax.z = (aabb[sign.z].z - ray.origin.z) * invRayDirection.z;

	if (tMax.x < tMax.y)
	{
		if (tMax.x < tMax.z)
		{
			newT = tMax.x;
		}
		else
		{
			newT = tMax.z;
		}
	}
	else
	{
		if (tMax.y < tMax.z)
		{
			newT = tMax.y;
		}
		else
		{
			newT = tMax.z;
		}
	}

	newT += 0.1f;
	
	res = WorldSpaceToVoxelSpace(GetPositionAlongRay(ray, newT));

	return res;
}

int3 GoToNextVoxel(in Ray ray, in float3 voxelPos, in float cellSize, out float newT, out float3 voxelFaceNormal)
{
	float3 tMax = float3(100000, 100000, 100000);
	int3 res = int3(0,0,0);
	const float FLT_INFINITY = 1.#INF;
	float3 invRayDirection = ray.direction != 0 ? 1 / ray.direction : (ray.direction > 0) ? FLT_INFINITY : -FLT_INFINITY;

	int3 rayDir = ray.direction > 0;
	int3 voxelDir = sign(ray.direction);
	
	float3 aabb[2] = { voxelPos, voxelPos + cellSize };

	if(ray.direction.x != 0)
		tMax.x = (aabb[rayDir.x].x - ray.origin.x) * invRayDirection.x;
	
	if(ray.direction.y != 0)
		tMax.y = (aabb[rayDir.y].y - ray.origin.y) * invRayDirection.y;
	
	if(ray.direction.z != 0)
		tMax.z = (aabb[rayDir.z].z - ray.origin.z) * invRayDirection.z;

	if (tMax.x < tMax.y)
	{
		if (tMax.x < tMax.z)
		{
			newT = tMax.x;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[0] * -voxelDir.x;
		}
		else
		{
			newT = tMax.z;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[4] * -voxelDir.z;
		}
	}
	else
	{
		if (tMax.y < tMax.z)
		{
			newT = tMax.y;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[2] * -voxelDir.y;
		}
		else
		{
			newT = tMax.z;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[4] * -voxelDir.z;
		}
	}

	newT += 0.1f;
	
	res = WorldSpaceToVoxelSpace(GetPositionAlongRay(ray, newT));

	return res;
}

float CalculateCellExit(in Ray ray, in float3 cellPos, in float cellSize)
{
	float3 tMax = float3(100000, 100000, 100000);
	float newT = 0;
	const float FLT_INFINITY = 1.#INF;
	float3 invRayDirection = ray.direction != 0 ? 1 / ray.direction : (ray.direction > 0) ? FLT_INFINITY : -FLT_INFINITY;

	int3 sign = ray.direction > 0;

	float3 voxelExtends = cellSize * 0.5;
	float3 aabb[2] = { cellPos, cellPos + cellSize };

	if (ray.direction.x != 0)
		tMax.x = (aabb[sign.x].x - ray.origin.x) * invRayDirection.x;

	if (ray.direction.y != 0)
		tMax.y = (aabb[sign.y].y - ray.origin.y) * invRayDirection.y;

	if (ray.direction.z != 0)
		tMax.z = (aabb[sign.z].z - ray.origin.z) * invRayDirection.z;

	if (tMax.x < tMax.y)
	{
		if (tMax.x < tMax.z)
		{
			newT = tMax.x;
		}
		else
		{
			newT = tMax.z;
		}
	}
	else
	{
		if (tMax.y < tMax.z)
		{
			newT = tMax.y;
		}
		else
		{
			newT = tMax.z;
		}
	}

	return newT;
}



bool IsValidVoxelIndex(in int3 index)
{
	uint instance = InstanceID();
	return index.x >= 0 && index.x < g_geometryCB[instance].voxelAxisCount && index.y >= 0 && index.y < g_geometryCB[instance].voxelAxisCount && index.z >= 0 && index.z < g_geometryCB[instance].voxelAxisCount;
}

int3 GetOctreeNodeIndex(in int3 parentIndex, in int3 relativeIndex, in int nodeCount)
{
	int3 nodeCountVec = int3(nodeCount, nodeCount, nodeCount);

	return parentIndex + nodeCountVec - (nodeCountVec / (relativeIndex + 1));
}

inline bool IsLeaf(in int3 octreeNodeIndex)
{
	uint instance = InstanceID();

	return g_voxelVolume[instance][octreeNodeIndex].a == 1;
}

float DecodeDensity(int2 data)
{
	int rMask = 0x7f;
	int gMask = 0xff;

	int iRes = (data.x & rMask) << 8;
	iRes |= (data.y & gMask);

	float res = (float)iRes * 0.01;

	return ((data.x & 0x80) == 0x80) ? -res : res;
}

inline float GetVoxelDensity(in int3 voxelPos, in uint instanceID)
{
	return DecodeDensity(g_voxelVolume[instanceID][voxelPos].rg);
}

inline int3 GetChildNodePtr(in uint instanceID, in int3 parentNodePtr, in int3 childIndex)
{
	return g_voxelVolume[instanceID][parentNodePtr + childIndex].rgb;
}

inline bool IsCellIndexInsideOctreeNode(in int3 parentIndex, in int3 minNodeIndex, in int3 cellIndex, in int depth, in int maxDepth, in int nodeCount)
{
	if(maxDepth == depth)
	{
		return minNodeIndex.x == cellIndex.x && minNodeIndex.y == cellIndex.y && minNodeIndex.z == cellIndex.z;		  
	}
	else
	{
		int3 maxNodeIndex = minNodeIndex + /*int3(1,1,1) * pow(2, maxDepth - (depth + 1))*/ nodeCount / 2;

		bool3 b = cellIndex >= minNodeIndex && cellIndex < maxNodeIndex;

		return b.x && b.y && b.z;		  
	}
}

VoxelOctreeNode GetOctreeNode(in int3 cellIndex)
{
	VoxelOctreeNode res =
	{
		0,
		float3(-1,-1,-1)
	};

	if (IsValidCell(cellIndex))
	{
		uint instanceID = InstanceID();

		int maxDepth = g_geometryCB[instanceID].octreeDepth;
		int3 currentNodeIndex = int3(0,0,0);
		int3 currentNodePtr = int3(0,0,0);
		
		if (g_traversalVolume[instanceID][int3(0, 0, 0)].a == 1)
		{
			res.size = GetNodeSize(0);
			res.nodePos = float3(1, 1, 1) * res.size * -0.5;
			return res;
		}
		
		for(int currentDepth = 0; currentDepth <= 8; currentDepth++)
		{
			uint4 n1 = g_traversalVolume[instanceID][currentNodePtr].rgba;
			uint4 n2 = g_traversalVolume[instanceID][currentNodePtr + int3(1,0,0)].rgba;
			uint4 n3 = g_traversalVolume[instanceID][currentNodePtr + int3(0,1,0)].rgba;
			uint4 n4 = g_traversalVolume[instanceID][currentNodePtr + int3(1,1,0)].rgba;
			uint4 n5 = g_traversalVolume[instanceID][currentNodePtr + int3(0,0,1)].rgba;
			uint4 n6 = g_traversalVolume[instanceID][currentNodePtr + int3(1,0,1)].rgba;
			uint4 n7 = g_traversalVolume[instanceID][currentNodePtr + int3(0,1,1)].rgba;
			uint4 n8 = g_traversalVolume[instanceID][currentNodePtr + int3(1,1,1)].rgba;
			
			int3 childNodeIndex = int3(0, 0, 0);
			int nodeCount = pow(2, maxDepth - currentDepth);
			
			int3 minNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, nodeCount);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, minNodeIndex, cellIndex, currentDepth, maxDepth, nodeCount))
			{
				if (n1.a == 1)
				{
					res.size = GetNodeSize(currentDepth);
					res.nodePos = VoxelIndexToWorldSpace(n1.rgb);
						
					return res;
				}
				else
				{
					currentNodePtr = n1.rgb;
					currentNodeIndex = minNodeIndex;
					continue;
				}
			}

			childNodeIndex = int3(1, 0, 0);
			minNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, nodeCount);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, minNodeIndex, cellIndex, currentDepth, maxDepth, nodeCount))
			{
				if (n2.a == 1)
				{
					res.size = GetNodeSize(currentDepth);
					res.nodePos = VoxelIndexToWorldSpace(n2.rgb);
						
					return res;
				}
				else
				{
					currentNodePtr = n2.rgb;
					currentNodeIndex = minNodeIndex;
					continue;
				}
			}

			childNodeIndex = int3(0, 1, 0);
			minNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, nodeCount);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, minNodeIndex, cellIndex, currentDepth, maxDepth, nodeCount))
			{
				if (n3.a == 1)
				{
					res.size = GetNodeSize(currentDepth);
					res.nodePos = VoxelIndexToWorldSpace(n3.rgb);
						
					return res;
				}
				else
				{
					currentNodePtr = n3.rgb;
					currentNodeIndex = minNodeIndex;
					continue;
				}
			}

			childNodeIndex = int3(1, 1, 0);
			minNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, nodeCount);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, minNodeIndex, cellIndex, currentDepth, maxDepth, nodeCount))
			{
				if (n4.a == 1)
				{
					res.size = GetNodeSize(currentDepth);
					res.nodePos = VoxelIndexToWorldSpace(n4.rgb);
						
					return res;
				}
				else
				{
					currentNodePtr = n4.rgb;
					currentNodeIndex = minNodeIndex;
					continue;
				}
			}

			childNodeIndex = int3(0, 0, 1);
			minNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, nodeCount);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, minNodeIndex, cellIndex, currentDepth, maxDepth, nodeCount))
			{
				if (n5.a == 1)
				{
					res.size = GetNodeSize(currentDepth);
					res.nodePos = VoxelIndexToWorldSpace(n5.rgb);
						
					return res;
				}
				else
				{
					currentNodePtr = n5.rgb;
					currentNodeIndex = minNodeIndex;
					continue;
				}
			}

			childNodeIndex = int3(1, 0, 1);
			minNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, nodeCount);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, minNodeIndex, cellIndex, currentDepth, maxDepth, nodeCount))
			{
				if (n6.a == 1)
				{
					res.size = GetNodeSize(currentDepth);
					res.nodePos = VoxelIndexToWorldSpace(n6.rgb);
						
					return res;
				}
				else
				{
					currentNodePtr = n6.rgb;
					currentNodeIndex = minNodeIndex;
					continue;
				}
			}

			childNodeIndex = int3(0, 1, 1);
			minNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, nodeCount);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, minNodeIndex, cellIndex, currentDepth, maxDepth, nodeCount))
			{
				if (n7.a == 1)
				{
					res.size = GetNodeSize(currentDepth);
					res.nodePos = VoxelIndexToWorldSpace(n7.rgb);
						
					return res;
				}
				else
				{
					currentNodePtr = n7.rgb;
					currentNodeIndex = minNodeIndex;
					continue;
				}
			}

			childNodeIndex = int3(1, 1, 1);
			minNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, nodeCount);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, minNodeIndex, cellIndex, currentDepth, maxDepth, nodeCount))
			{
				if (n8.a == 1)
				{
					res.size = GetNodeSize(currentDepth);
					res.nodePos = VoxelIndexToWorldSpace(n8.rgb);
						
					return res;
				}
				else
				{
					currentNodePtr = n8.rgb;
					currentNodeIndex = minNodeIndex;
					continue;
				}
			}
		}
	}
	
	//res.depth = 1;
	//res.size = GetNodeSize(res.depth);

	return res;	 
}

bool HasIsoSurfaceInsideCell(in uint instanceID, in int3 cellIndex)
{
	float v1 = GetVoxelDensity(cellIndex, instanceID);
	float v2 = GetVoxelDensity(cellIndex + int3(1,0,0), instanceID);
	float v3 = GetVoxelDensity(cellIndex + int3(0,1,0), instanceID);
	float v4 = GetVoxelDensity(cellIndex + int3(1,1,0), instanceID);
	float v5 = GetVoxelDensity(cellIndex + int3(0,0,1), instanceID);
	float v6 = GetVoxelDensity(cellIndex + int3(1,0,1), instanceID);
	float v7 = GetVoxelDensity(cellIndex + int3(0,1,1), instanceID);
	float v8 = GetVoxelDensity(cellIndex + int3(1,1,1), instanceID);
	
	float v1Sign = sign(v1);

	return  v1Sign != sign(v2) ||
			v1Sign != sign(v3) ||
			v1Sign != sign(v4) ||
			v1Sign != sign(v5) ||
			v1Sign != sign(v6) ||
			v1Sign != sign(v7) ||
			v1Sign != sign(v8);
}

inline bool IsSolidCell(in int3 cellIndex, in uint instanceID)
{
	float v1 = GetVoxelDensity(cellIndex, instanceID);
	float v2 = GetVoxelDensity(cellIndex + int3(1,0,0), instanceID);
	float v3 = GetVoxelDensity(cellIndex + int3(0,1,0), instanceID);
	float v4 = GetVoxelDensity(cellIndex + int3(1,1,0), instanceID);
	float v5 = GetVoxelDensity(cellIndex + int3(0,0,1), instanceID);
	float v6 = GetVoxelDensity(cellIndex + int3(1,0,1), instanceID);
	float v7 = GetVoxelDensity(cellIndex + int3(0,1,1), instanceID);
	float v8 = GetVoxelDensity(cellIndex + int3(1,1,1), instanceID);
	
	return	v1 < 0.f &&
			v2 < 0.f &&
			v3 < 0.f &&
			v4 < 0.f &&
			v5 < 0.f &&
			v6 < 0.f &&
			v7 < 0.f &&
			v8 < 0.f;
}

inline float SumUVW(in float u0, in float v0, in float w0, in float u1, in float v1, in float w1)
{
	return	u0 * v0 * w0 +
			u1 * v0 * w0 +
			u0 * v1 * w0 +
			u1 * v1 * w0 +
			u0 * v0 * w1 +
			u1 * v0 * w1 +
			u0 * v1 * w1 +
			u1 * v1 * w1;
}

void GetDensityPolynomial(in Ray ray, in int3 cellIndex, in float cellSize, in float tIn, in float tOut, out float A, out float B, out float C, out float D)
{
	uint instanceID = InstanceID();
	
	float3 a1 = WorldSpaceToBottomLevelCellSpace(cellIndex, cellSize, GetPositionAlongRay(ray, tIn));
	float3 a0 = 1 - a1;
	float3 b1 = WorldSpaceToBottomLevelCellSpace(cellIndex, cellSize, GetPositionAlongRay(ray, tOut)) - a1;
	float3 b0 = -b1;

	float v1 = GetVoxelDensity(cellIndex, instanceID);
	float v2 = GetVoxelDensity(cellIndex + int3(1,0,0), instanceID);
	float v3 = GetVoxelDensity(cellIndex + int3(0,1,0), instanceID);
	float v4 = GetVoxelDensity(cellIndex + int3(1,1,0), instanceID);
	float v5 = GetVoxelDensity(cellIndex + int3(0,0,1), instanceID);
	float v6 = GetVoxelDensity(cellIndex + int3(1,0,1), instanceID);
	float v7 = GetVoxelDensity(cellIndex + int3(0,1,1), instanceID);
	float v8 = GetVoxelDensity(cellIndex + int3(1,1,1), instanceID);
	
	A =	b0.x * b0.y * b0.z * v1 + 
		b1.x * b0.y * b0.z * v2 +
		b0.x * b1.y * b0.z * v3 +
		b1.x * b1.y * b0.z * v4 +
		b0.x * b0.y * b1.z * v5 +
		b1.x * b0.y * b1.z * v6 +
		b0.x * b1.y * b1.z * v7 +
		b1.x * b1.y * b1.z * v8;

	D = a0.x * a0.y * a0.z * v1 +
		a1.x * a0.y * a0.z * v2 +
		a0.x * a1.y * a0.z * v3 +
		a1.x * a1.y * a0.z * v4 +
		a0.x * a0.y * a1.z * v5 +
		a1.x * a0.y * a1.z * v6 +
		a0.x * a1.y * a1.z * v7 +
		a1.x * a1.y * a1.z * v8;

	B = (a0.x * b0.y * b0.z + b0.x * a0.y * b0.z + b0.x * b0.y * a0.z) * v1 +
		(a1.x * b0.y * b0.z + b1.x * a0.y * b0.z + b1.x * b0.y * a0.z) * v2 +
		(a0.x * b1.y * b0.z + b0.x * a1.y * b0.z + b0.x * b1.y * a0.z) * v3 +
		(a1.x * b1.y * b0.z + b1.x * a1.y * b0.z + b1.x * b1.y * a0.z) * v4 +
		(a0.x * b0.y * b1.z + b0.x * a0.y * b1.z + b0.x * b0.y * a1.z) * v5 +
		(a1.x * b0.y * b1.z + b1.x * a0.y * b1.z + b1.x * b0.y * a1.z) * v6 +
		(a0.x * b1.y * b1.z + b0.x * a1.y * b1.z + b0.x * b1.y * a1.z) * v7 +
		(a1.x * b1.y * b1.z + b1.x * a1.y * b1.z + b1.x * b1.y * a1.z) * v8;

	C = (b0.x * a0.y * a0.z + a0.x * b0.y * a0.z + a0.x * a0.y * b0.z) * v1 +
		(b1.x * a0.y * a0.z + a1.x * b0.y * a0.z + a1.x * a0.y * b0.z) * v2 +
		(b0.x * a1.y * a0.z + a0.x * b1.y * a0.z + a0.x * a1.y * b0.z) * v3 +
		(b1.x * a1.y * a0.z + a1.x * b1.y * a0.z + a1.x * a1.y * b0.z) * v4 +
		(b0.x * a0.y * a1.z + a0.x * b0.y * a1.z + a0.x * a0.y * b1.z) * v5 +
		(b1.x * a0.y * a1.z + a1.x * b0.y * a1.z + a1.x * a0.y * b1.z) * v6 +
		(b0.x * a1.y * a1.z + a0.x * b1.y * a1.z + a0.x * a1.y * b1.z) * v7 +
		(b1.x * a1.y * a1.z + a1.x * b1.y * a1.z + a1.x * a1.y * b1.z) * v8;
}

float GetDensity(in uint instanceID, in int3 cellIndex, in float3 cellPos)
{
	float v1 = GetVoxelDensity(cellIndex, instanceID);
	float v2 = GetVoxelDensity(cellIndex + int3(1,0,0), instanceID);
	float v3 = GetVoxelDensity(cellIndex + int3(0,1,0), instanceID);
	float v4 = GetVoxelDensity(cellIndex + int3(1,1,0), instanceID);
	float v5 = GetVoxelDensity(cellIndex + int3(0,0,1), instanceID);
	float v6 = GetVoxelDensity(cellIndex + int3(1,0,1), instanceID);
	float v7 = GetVoxelDensity(cellIndex + int3(0,1,1), instanceID);
	float v8 = GetVoxelDensity(cellIndex + int3(1,1,1), instanceID);
	
	float p = 0.f;

	float u = abs((1 - 0) - cellPos.x);
	float v = abs((1 - 0) - cellPos.y);
	float w = abs((1 - 0) - cellPos.z);

	p += (u * v * w * v1);

	u = abs((1 - 1) - cellPos.x);
	v = abs((1 - 0) - cellPos.y);
	w = abs((1 - 0) - cellPos.z);

	p += (u * v * w * v2);

	u = abs((1 - 0) - cellPos.x);
	v = abs((1 - 1) - cellPos.y);
	w = abs((1 - 0) - cellPos.z);

	p += (u * v * w * v3);

	u = abs((1 - 1) - cellPos.x);
	v = abs((1 - 1) - cellPos.y);
	w = abs((1 - 0) - cellPos.z);

	p += (u * v * w * v4);

	u = abs((1 - 0) - cellPos.x);
	v = abs((1 - 0) - cellPos.y);
	w = abs((1 - 1) - cellPos.z);

	p += (u * v * w * v5);

	u = abs((1 - 1) - cellPos.x);
	v = abs((1 - 0) - cellPos.y);
	w = abs((1 - 1) - cellPos.z);

	p += (u * v * w * v6);

	u = abs((1 - 0) - cellPos.x);
	v = abs((1 - 1) - cellPos.y);
	w = abs((1 - 1) - cellPos.z);

	p += (u * v * w * v7);

	u = abs((1 - 1) - cellPos.x);
	v = abs((1 - 1) - cellPos.y);
	w = abs((1 - 1) - cellPos.z);

	p += (u * v * w * v8);

	//for (int i = 0; i <= 1; i++)
	//{
	//	for (int j = 0; j <= 1; j++)
	//	{
	//		for (int k = 0; k <= 1; k++)
	//		{
	//			float u = abs((1 - i) - cellPos.x);
	//			float v = abs((1 - j) - cellPos.y);
	//			float w = abs((1 - k) - cellPos.z);

	//			p += (u * v * w * g_voxelVolume[instance][cellOriginIndex + int3(i, j, k)]);
	//		}
	//	}
	//}

	return p;
}

inline float GetDensityWithPolynomial(in float t, in float A, in float B, in float C, in float D)
{
	return A*t*t*t + B*t*t + C*t + D;
}

bool GetSurfaceIntersectionT(in Ray ray, in int3 cellIndex, in float cellSize, in float tIn, in float tOut, out float tHit)
{
	float A, B, C, D = 0;
	float t0 = max(0, -tIn / (tOut - tIn));
	float t1 = 1;

	GetDensityPolynomial(ray, cellIndex, cellSize, tIn, tOut, A, B, C, D);

	float dA = 3 * A;
	float dB = 2 * B;

	float ex1 = (-dB + sqrt(dB*dB - 4*dA*C)) / (2 * dA);
	float ex2 = (-dB - sqrt(dB*dB - 4*dA*C)) / (2 * dA);

	float f0 = GetDensityWithPolynomial(t0, A, B, C, D);

	if (sign(f0) <= 0)
	{
		tHit = tIn;
		return true;
	}

	float f1 = GetDensityWithPolynomial(t1, A, B, C, D);

	if (ex1 > ex2)
	{
		float f = ex1;
		ex1 = ex2;
		ex2 = f;
	}

	if (ex1 >= t0 && ex1 <= t1)
	{
		float fex1 = GetDensityWithPolynomial(ex1, A, B, C, D);

		if (sign(fex1) == sign(f0))
		{
			t0 = ex1;
			f0 = fex1;
		}
		else
		{
			t1 = ex1;
			f1 = fex1;
		}
	}

	if (ex2 >= t0 && ex2 <= t1)
	{
		float fex2 = GetDensityWithPolynomial(ex2, A, B, C, D);

		if (sign(fex2) == sign(f0))
		{
			t0 = ex2;
			f0 = fex2;
		}
		else
		{
			t1 = ex2;
			f1 = fex2;
		}

	}

	if (sign(f0) == sign(f1))
	{
		return false;
	}

	for (int i = 0; i < 2; i++)
	{
		float t = t0 + (t1 - t0) * (-f0 / (f1 - f0));
		float f = GetDensityWithPolynomial(t, A, B, C, D);

		if (sign(f) == sign(f0))
		{
			t0 = t;
			f0 = f;
		}
		else
		{
			t1 = t;
			f1 = f;
		}
	}

	tHit = t0 + (t1 - t0) * (-f0 / (f1 - f0));
	tHit = lerp(tIn, tOut, tHit);

	return tHit > 0;
}

float3 GetNormal(in int3 cellIndex, in float3 normPos)
{
	uint instance = InstanceID();

	float distanceBtwVoxel = g_geometryCB[instance].distanceBtwVoxels;

	float x = (GetDensity(instance, cellIndex + int3(1,0,0), normPos) - GetDensity(instance, cellIndex - int3(1,0,0), normPos));// / (/*2 **/ distanceBtwVoxel);
	float y = (GetDensity(instance, cellIndex + int3(0,1,0), normPos) - GetDensity(instance, cellIndex - int3(0,1,0), normPos));// / (/*2 **/ distanceBtwVoxel);
	float z = (GetDensity(instance, cellIndex + int3(0,0,1), normPos) - GetDensity(instance, cellIndex - int3(0,0,1), normPos));// / (/*2 **/ distanceBtwVoxel);

	float3 norm = float3(x,y,z);
	bool3 nan = isnan(norm);

	if (nan.x || nan.y || nan.z)
	{
		return float3(0, 0, 0);
	}
	else
	{
		return normalize(norm);
	}
}