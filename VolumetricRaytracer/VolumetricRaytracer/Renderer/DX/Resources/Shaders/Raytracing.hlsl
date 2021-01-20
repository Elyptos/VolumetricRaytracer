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

#define HLSL

#include "RaytracingHlsl.h"

RaytracingAccelerationStructure g_scene : register(t0, space0);

Texture3D<uint4> g_voxelVolume[VolumeRaytracer::MaxAllowedObjectData] : register(t2, space0);
Texture3D<uint4> g_traversalVolume[VolumeRaytracer::MaxAllowedObjectData] : register(t22, space0);
ConstantBuffer<VolumeRaytracer::VPointLightBuffer> g_pointLightsCB[VolumeRaytracer::MaxAllowedPointLights] : register(b1);
ConstantBuffer<VolumeRaytracer::VSpotLightBuffer> g_spotLightsCB[VolumeRaytracer::MaxAllowedSpotLights] : register(b6);
ConstantBuffer<VolumeRaytracer::VGeometryConstantBuffer> g_geometryCB[VolumeRaytracer::MaxAllowedObjectData] : register(b11);

TextureCube g_envMap : register(t1, space0);
SamplerState g_envMapSampler : register(s0);

RWTexture2D<float4> g_renderTarget : register(u0);
ConstantBuffer<VolumeRaytracer::VSceneConstantBuffer> g_sceneCB : register(b0);

static const float PI = 3.141592f;

struct Ray
{
	float3 origin;
	float3 direction;
};

struct VoxelOctreeNode
{
	float size;
	float3 nodePos;
};

inline Ray GetLocalRay()
{
	Ray ray;

	ray.origin = ObjectRayOrigin();
	ray.direction = ObjectRayDirection();

	return ray;
}

float3 GetPositionAlongRay(in Ray ray, in float t)
{
	return ray.origin + ray.direction * t;
}

inline Ray GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 viewInverted, in float4x4 projInverted)
{
	float2 xy = index + 0.5f; // center in the middle of the pixel.
	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	Ray ray;
	ray.origin = mul(viewInverted, float4(0,0,0,1)).xyz;

	float4 target = mul(projInverted, float4(screenPos.x, -screenPos.y, 1, 1));
	ray.direction = mul(viewInverted, float4(target.xyz, 0)).xyz;

	return ray;
}

inline Ray ReverseRay(in Ray ray)
{
	Ray res;

	res.direction = -ray.direction;
	res.origin = ray.origin;

	return ray;
}

float4 TraceRadianceRay(in Ray ray, in uint currentRayRecursionDepth)
{
	if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		return float4(0,0,0,0);
	}

	RayDesc rayDesc;
	rayDesc.Origin = ray.origin;
	rayDesc.Direction = ray.direction;

	rayDesc.TMin = 0;
	rayDesc.TMax = 10000;

	VolumeRaytracer::VRayPayload rayPayload = { float4(0,0,0,0), currentRayRecursionDepth + 1 };

	TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 2, 0, rayDesc, rayPayload);

	return rayPayload.color;
}

bool TraceShadowRay(in Ray ray, in uint currentRayRecursionDepth, in float maxDistance)
{
	if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		return false;
	}

	RayDesc rayDesc;
	rayDesc.Origin = ray.origin;
	rayDesc.Direction = ray.direction;

	rayDesc.TMin = 0;
	rayDesc.TMax = maxDistance;

	VolumeRaytracer::VShadowRayPayload rayPayload = { true };

	TraceRay(g_scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES
		| RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
		| RAY_FLAG_FORCE_OPAQUE             // ~skip any hit shaders
		| RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // ~skip closest hit shaders,
		0xff,
		1,
		2,
		1,
		rayDesc, rayPayload);

	return rayPayload.hit;
}

bool DetermineRayAABBIntersection(in Ray ray, in float3 aabb[2], out float tEnter, out float tExit)
{
	float3 tmin3, tmax3;
	int3 sign3 = ray.direction > 0;

	const float FLT_INFINITY = 1.#INF;
	float3 invRayDirection = ray.direction != 0
		? 1 / ray.direction
		: (ray.direction > 0) ? FLT_INFINITY : -FLT_INFINITY;

	tmin3.x = (aabb[1 - sign3.x].x - ray.origin.x) * invRayDirection.x;
	tmax3.x = (aabb[sign3.x].x - ray.origin.x) * invRayDirection.x;

	tmin3.y = (aabb[1 - sign3.y].y - ray.origin.y) * invRayDirection.y;
	tmax3.y = (aabb[sign3.y].y - ray.origin.y) * invRayDirection.y;

	tmin3.z = (aabb[1 - sign3.z].z - ray.origin.z) * invRayDirection.z;
	tmax3.z = (aabb[sign3.z].z - ray.origin.z) * invRayDirection.z;

	tEnter = max(max(tmin3.x, tmin3.y), tmin3.z);
	tExit = min(min(tmax3.x, tmax3.y), tmax3.z);

	return tExit > tEnter && tExit >= RayTMin() && tEnter <= RayTCurrent();
}

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

	res = WorldSpaceToVoxelSpace(GetPositionAlongRay(ray, newT + 0.1f));

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

int3 GetOctreeNodeIndex(in int3 parentIndex, in int3 relativeIndex, in int currentDepth)
{
	uint instance = InstanceID();

	int depthDiff = g_geometryCB[instance].octreeDepth - currentDepth;
	
	int nodeCount = pow(2, g_geometryCB[instance].octreeDepth - currentDepth);
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

inline bool IsCellIndexInsideOctreeNode(in int3 parentIndex, in int3 relOctreeNode, in int3 cellIndex, in int depth, in int maxDepth)
{
	int3 minNodeIndex = GetOctreeNodeIndex(parentIndex, relOctreeNode, depth);
	
	if(maxDepth == depth)
	{
		return minNodeIndex.x == cellIndex.x && minNodeIndex.y == cellIndex.y && minNodeIndex.z == cellIndex.z;		  
	}
	else
	{
		int3 maxNodeIndex = minNodeIndex + int3(1,1,1) * pow(2, maxDepth - (depth + 1));

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

		int currentDepth = -1;
		int maxDepth = g_geometryCB[instanceID].octreeDepth;
		int3 currentNodeIndex = int3(0,0,0);
		int3 currentNodePtr = int3(0,0,0);
		
		if (g_traversalVolume[instanceID][int3(0, 0, 0)].a == 1)
		{
			res.size = GetNodeSize(0);
			res.nodePos = float3(1, 1, 1) * res.size * -0.5;
			return res;
		}
		
		while (currentDepth <= maxDepth)
		{
			uint4 n1 = g_traversalVolume[instanceID][currentNodePtr].rgba;
			uint4 n2 = g_traversalVolume[instanceID][currentNodePtr + int3(1,0,0)].rgba;
			uint4 n3 = g_traversalVolume[instanceID][currentNodePtr + int3(0,1,0)].rgba;
			uint4 n4 = g_traversalVolume[instanceID][currentNodePtr + int3(1,1,0)].rgba;
			uint4 n5 = g_traversalVolume[instanceID][currentNodePtr + int3(0,0,1)].rgba;
			uint4 n6 = g_traversalVolume[instanceID][currentNodePtr + int3(1,0,1)].rgba;
			uint4 n7 = g_traversalVolume[instanceID][currentNodePtr + int3(0,1,1)].rgba;
			uint4 n8 = g_traversalVolume[instanceID][currentNodePtr + int3(1,1,1)].rgba;
			
			currentDepth += 1;
			
			int3 childNodeIndex = int3(0, 0, 0);
			
			if (IsCellIndexInsideOctreeNode(currentNodeIndex, childNodeIndex, cellIndex, currentDepth, maxDepth))
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
					currentNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, currentDepth);
					continue;
				}
			}

			childNodeIndex = int3(1, 0, 0);

			if (IsCellIndexInsideOctreeNode(currentNodeIndex, childNodeIndex, cellIndex, currentDepth, maxDepth))
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
					currentNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, currentDepth);
					continue;
				}
			}

			childNodeIndex = int3(0, 1, 0);

			if (IsCellIndexInsideOctreeNode(currentNodeIndex, childNodeIndex, cellIndex, currentDepth, maxDepth))
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
					currentNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, currentDepth);
					continue;
				}
			}

			childNodeIndex = int3(1, 1, 0);

			if (IsCellIndexInsideOctreeNode(currentNodeIndex, childNodeIndex, cellIndex, currentDepth, maxDepth))
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
					currentNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, currentDepth);
					continue;
				}
			}

			childNodeIndex = int3(0, 0, 1);

			if (IsCellIndexInsideOctreeNode(currentNodeIndex, childNodeIndex, cellIndex, currentDepth, maxDepth))
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
					currentNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, currentDepth);
					continue;
				}
			}

			childNodeIndex = int3(1, 0, 1);

			if (IsCellIndexInsideOctreeNode(currentNodeIndex, childNodeIndex, cellIndex, currentDepth, maxDepth))
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
					currentNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, currentDepth);
					continue;
				}
			}

			childNodeIndex = int3(0, 1, 1);

			if (IsCellIndexInsideOctreeNode(currentNodeIndex, childNodeIndex, cellIndex, currentDepth, maxDepth))
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
					currentNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, currentDepth);
					continue;
				}
			}

			childNodeIndex = int3(1, 1, 1);

			if (IsCellIndexInsideOctreeNode(currentNodeIndex, childNodeIndex, cellIndex, currentDepth, maxDepth))
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
					currentNodeIndex = GetOctreeNodeIndex(currentNodeIndex, childNodeIndex, currentDepth);
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

	//if (A == 0)
	//{
	//	return false;
	//}

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

	float x = (GetDensity(instance, cellIndex + int3(1,0,0), normPos) - GetDensity(instance, cellIndex - int3(1,0,0), normPos)) / (2 * distanceBtwVoxel);
	float y = (GetDensity(instance, cellIndex + int3(0,1,0), normPos) - GetDensity(instance, cellIndex - int3(0,1,0), normPos)) / (2 * distanceBtwVoxel);
	float z = (GetDensity(instance, cellIndex + int3(0,0,1), normPos) - GetDensity(instance, cellIndex - int3(0,0,1), normPos)) / (2 * distanceBtwVoxel);

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

[shader("raygeneration")]
void VRRaygen()
{
	Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, g_sceneCB.cameraPosition.xyz, g_sceneCB.viewMatrixInverted, g_sceneCB.projectionMatrixInverted);

	uint currentRecursionDepth = 0;
	float4 color = TraceRadianceRay(ray, currentRecursionDepth);

	g_renderTarget[DispatchRaysIndex().xy] = color;
}

float ComputePointLightIntensity(in float intensity, in float distance, in float attL, in float attExp)
{
	return intensity / (1 + attL * distance + attExp * distance * distance);
}

float CalculateConeFalloff(in float cosSurface, in float cosAngle, in float cosFalloffAngle)
{
	float delta = (cosSurface - cosAngle) / (cosFalloffAngle - cosAngle);
	
	return min(delta, 1.f);
}

float ComputeSpotLightIntensity(in float3 surfacePoint, in float distanceToSurface, in float3 lightPosition, in float3 lightDirection, in float intensity, in float attL, in float attExp, in float cosAngle, in float cosFalloffAngle)
{
	float3 surfaceDir = normalize(surfacePoint - lightPosition);
	float cosSurface = dot(lightDirection, surfaceDir);
	
	float calculatedIntensity = 0.f;
	
	if(cosSurface >= 0.f && cosSurface > cosAngle)
	{
		calculatedIntensity = intensity * CalculateConeFalloff(cosSurface, cosAngle, cosFalloffAngle);
		calculatedIntensity = ComputePointLightIntensity(calculatedIntensity, distanceToSurface, attL, attExp);
		
		return calculatedIntensity;
		//return 100.f;
	}
	else
	{
		return 0.f;		  
	}
}

[shader("closesthit")]
void VRClosestHit(inout VolumeRaytracer::VRayPayload rayPayload, in VolumeRaytracer::VPrimitiveAttributes attr)
{
	float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
	float3 shadowRayOrigin = hitPosition - WorldRayDirection() * 0.1f;

	//Trace for directional light
	Ray shadowRay;
	shadowRay.origin = shadowRayOrigin;
	shadowRay.direction = g_sceneCB.dirLightDirection;

	bool shadowRayHit = TraceShadowRay(shadowRay, rayPayload.depth, 600.f);
	
	float diffuse = 0.1f;
	
	//if(!shadowRayHit)
	//{
	//	diffuse = (0.5 / PI) * g_sceneCB.dirLightStrength * dot(attr.normal, g_sceneCB.dirLightDirection);	  
	//}

	float distanceToLightSource = 0;
	float lightIntensity = 0;
	
	for (int pi = 0; pi < g_sceneCB.numPointLights; pi++)
	{
		distanceToLightSource = length(g_pointLightsCB[pi].position - shadowRayOrigin);
		lightIntensity = ComputePointLightIntensity(g_pointLightsCB[pi].lightIntensity, distanceToLightSource, g_pointLightsCB[pi].attLinear, g_pointLightsCB[pi].attExp);
		
		if(lightIntensity > 0.f)
		{
			shadowRay.direction = normalize(g_pointLightsCB[pi].position - shadowRayOrigin);
			shadowRayHit = TraceShadowRay(shadowRay, rayPayload.depth, distanceToLightSource);
		
			if(!shadowRayHit)
			{
				diffuse += lightIntensity;
			}	 
		}
	}
	
	for(int si = 0; si < g_sceneCB.numSpotLights; si++)
	{
		distanceToLightSource = length(g_spotLightsCB[si].position - shadowRayOrigin);
		lightIntensity = ComputeSpotLightIntensity(shadowRayOrigin, distanceToLightSource, g_spotLightsCB[si].position, g_spotLightsCB[si].forward, g_spotLightsCB[si].lightIntensity, g_spotLightsCB[si].attLinear, g_spotLightsCB[si].attExp, g_spotLightsCB[si].cosAngle, g_spotLightsCB[si].cosFalloffAngle);
		
		if(lightIntensity > 0.f)
		{
			shadowRay.direction = normalize(g_spotLightsCB[si].position - shadowRayOrigin);
			shadowRayHit = TraceShadowRay(shadowRay, rayPayload.depth, distanceToLightSource);
		
			if(!shadowRayHit)
			{
				diffuse += lightIntensity;
			}	 
		}
	}
	
	rayPayload.color.rgb = float3(1.f, 1.f, 1.f) * (diffuse * 0.01f);
	//rayPayload.color.rgb = attr.normal;
	rayPayload.color.a = 1.f;
}

[shader("intersection")]
void VRIntersection()
{
	uint instance = InstanceID();
	
	Ray ray;

	ray.origin = WorldRayOrigin();
	ray.direction = WorldRayDirection();

	Ray localRay = GetLocalRay();

	//Check if we intersect voxel volume
	float3 volumeAABB[2] = {float3(-g_geometryCB[instance].volumeExtend, -g_geometryCB[instance].volumeExtend, -g_geometryCB[instance].volumeExtend), float3(g_geometryCB[instance].volumeExtend, g_geometryCB[instance].volumeExtend, g_geometryCB[instance].volumeExtend)};
	float tEnter, tExit;

	float tMax = RayTCurrent();

	if (DetermineRayAABBIntersection(localRay, volumeAABB, tEnter, tExit))
	{
		int3 nextVoxelPos;
		int3 currentVoxelPos;
		float cellExit;
		float cellEnter;
		float tHit;
		
		VoxelOctreeNode octreeNode;

		if (tEnter >= 0)
		{
			tEnter += 0.01;

			currentVoxelPos = WorldSpaceToVoxelSpace(GetPositionAlongRay(localRay, tEnter));
			cellExit = tEnter;

			octreeNode = GetOctreeNode(currentVoxelPos);
			
			//if(octreeNode.nodePos.x == -100)
			//{
			//	VolumeRaytracer::VPrimitiveAttributes attr;
			//	attr.normal = float3(1, 1, 1);

			//	ReportHit(10, 0, attr);

			//	return;
			//}
		}
		else
		{
			currentVoxelPos = WorldSpaceToVoxelSpace(localRay.origin);

			octreeNode = GetOctreeNode(currentVoxelPos);

			//Cell exit is behind the ray origin, to calculate it we traverse in the opposite direction.
			cellExit = CalculateCellExit(ReverseRay(localRay), octreeNode.nodePos, octreeNode.size);

			cellExit = -cellExit;
			cellExit += 0.01;
		}

		int maxIterations = 3000;

		if (IsValidCell(currentVoxelPos) && IsSolidCell(currentVoxelPos, instance))
		{
			float3 rayPos = GetPositionAlongRay(localRay, tEnter - 0.1);

			VolumeRaytracer::VPrimitiveAttributes attr;
			attr.normal = sign(rayPos - volumeAABB[1]);

			if (attr.normal.x < 0)
			{
				attr.normal.x = rayPos.x < volumeAABB[0].x ? -1 : 0;
			}

			if (attr.normal.y < 0)
			{
				attr.normal.y = rayPos.y < volumeAABB[0].y ? -1 : 0;
			}

			if (attr.normal.z < 0)
			{
				attr.normal.z = rayPos.z < volumeAABB[0].z ? -1 : 0;
			}

			attr.normal = normalize(attr.normal);

			ReportHit(tEnter, 0, attr);

			return;
		}

		while (cellExit <= tExit && maxIterations > 0)
		{
			maxIterations--;

			cellEnter = cellExit;

			if (IsValidCell(currentVoxelPos))
			{
				nextVoxelPos = GoToNextVoxel(localRay, octreeNode.nodePos, octreeNode.size, cellExit);
				
				//if (cell.v8 == -1.f)
				//{
				//	VolumeRaytracer::VPrimitiveAttributes attr;
				//	attr.normal = float3(1, 1, 1);

				//	ReportHit(10, 0, attr);

				//	return;
				//}

				if (HasIsoSurfaceInsideCell(instance, currentVoxelPos))
				{
					//VolumeRaytracer::VPrimitiveAttributes attr;
					//attr.normal = float3(1, 1, 1);

					//ReportHit(10, 0, attr);

					//return;

					if (GetSurfaceIntersectionT(localRay, currentVoxelPos, octreeNode.size, cellEnter, cellExit, tHit))
					{
						VolumeRaytracer::VPrimitiveAttributes attr;
						attr.normal = GetNormal(currentVoxelPos, WorldSpaceToBottomLevelCellSpace(currentVoxelPos, octreeNode.size, GetPositionAlongRay(localRay, tHit)));

						ReportHit(tHit, 0, attr);
					}
					//else
					//{
					//	VolumeRaytracer::VPrimitiveAttributes attr;
					//	attr.normal = float3(1.f, 1.f, 1.f);

					//	ReportHit(tHit, 0, attr);
					//}
				}
			}
			else
			{
				//nextVoxelPos = GoToNextVoxel(localRay, nextVoxelPos, g_geometryCB[instance].distanceBtwVoxels, g_geometryCB[instance].octreeDepth, cellExit);

				//VolumeRaytracer::VPrimitiveAttributes attr;
				//attr.normal = float3(0, 1, 0);

				//ReportHit(10, 0, attr);

				//return;
				
				break;
			}

			currentVoxelPos = nextVoxelPos;
			octreeNode = GetOctreeNode(nextVoxelPos);
		}
		
		if (maxIterations <= 0)
		{
			VolumeRaytracer::VPrimitiveAttributes attr;
			attr.normal = float3(1, 0, 0);

			ReportHit(10, 0, attr);

			return;
		}
	}
}

[shader("miss")]
void VRMiss(inout VolumeRaytracer::VRayPayload rayPayload)
{
	float3 rayDirection = WorldRayDirection();
	rayPayload.color = g_envMap.SampleLevel(g_envMapSampler, rayDirection.xzy, 0);
}

[shader("miss")]
void VRMissShadowRay(inout VolumeRaytracer::VShadowRayPayload rayPayload)
{
	rayPayload.hit = false;
}