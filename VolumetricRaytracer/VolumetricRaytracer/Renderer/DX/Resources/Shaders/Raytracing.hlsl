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
Texture3D<float4> g_voxelVolume : register(t2, space0);

TextureCube g_envMap : register(t1, space0);
SamplerState g_envMapSampler : register(s0);

RWTexture2D<float4> g_renderTarget : register(u0);
ConstantBuffer<VolumeRaytracer::VSceneConstantBuffer> g_sceneCB : register(b0);

struct Ray
{
	float3 origin;
	float3 direction;
};

inline Ray GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 viewInverted, in float4x4 projInverted)
{
	float2 xy = index + 0.5f; // center in the middle of the pixel.
	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates.
	//screenPos.y = -screenPos.y;

	// Unproject the pixel coordinate into a world positon.
	//float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);
	//world.xyz /= world.w;

	Ray ray;
	//ray.origin = cameraPosition;
	ray.origin = mul(viewInverted, float4(0,0,0,1)).xyz;

	float4 target = mul(projInverted, float4(screenPos.x, -screenPos.y, 1, 1));
	ray.direction = mul(viewInverted, float4(target.xyz, 0)).xyz;

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
	rayDesc.TMax = 1000;

	VolumeRaytracer::VRayPayload rayPayload = { float4(0,0,0,0), currentRayRecursionDepth + 1 };

	TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 2, 0, rayDesc, rayPayload);

	return rayPayload.color;
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
	float3 volumeOrigin = -g_sceneCB.volumeExtend;
	float3 relativeVolumePos = worldLocation - volumeOrigin;
	float distanceBetweenVoxel = g_sceneCB.distanceBtwVoxels;

	return int3(floor(relativeVolumePos.x / distanceBetweenVoxel), floor(relativeVolumePos.y / distanceBetweenVoxel), floor(relativeVolumePos.z / distanceBetweenVoxel));
}

float3 VoxelIndexToWorldSpace(in int3 voxelIndex)
{
	float distanceBetweenVoxel = g_sceneCB.distanceBtwVoxels;
	float3 voxelIndexF = voxelIndex;
	float3 volumeOrigin = -g_sceneCB.volumeExtend;

	return voxelIndexF * distanceBetweenVoxel + volumeOrigin;
}

inline float3 WorldSpaceToCellSpace(in int3 cellOriginPos, in float3 worldLocation)
{
	float3 voxelOriginPos = VoxelIndexToWorldSpace(cellOriginPos);
	float3 rel = worldLocation - voxelOriginPos;

	return rel / g_sceneCB.distanceBtwVoxels;
}

bool IsValidCell(in int3 originVoxelPos)
{
	return  originVoxelPos.x >= 0 &&
			originVoxelPos.y >= 0 &&
			originVoxelPos.z >= 0 &&
			(originVoxelPos.x + 1) < g_sceneCB.voxelAxisCount &&
			(originVoxelPos.y + 1) < g_sceneCB.voxelAxisCount &&
			(originVoxelPos.z + 1) < g_sceneCB.voxelAxisCount;
}

int3 GoToNextVoxel(in Ray ray, in uint3 voxelIndex, in int3 direction, out float newT, out float3 voxelFaceNormal)
{
	float3 tMax = float3(100000, 100000, 100000);
	int3 res = voxelIndex;
	const float FLT_INFINITY = 1.#INF;
	float3 invRayDirection = ray.direction != 0 ? 1 / ray.direction : (ray.direction > 0) ? FLT_INFINITY : -FLT_INFINITY;

	int3 sign = ray.direction > 0;

	float3 voxelPos = VoxelIndexToWorldSpace(voxelIndex);
	float3 voxelExtends = g_sceneCB.distanceBtwVoxels * 0.5;
	float3 aabb[2] = { voxelPos, voxelPos + g_sceneCB.distanceBtwVoxels };

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
			res.x += direction.x;
			newT = tMax.x;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[0] * -direction.x;
		}
		else
		{
			res.z += direction.z;
			newT = tMax.z;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[4] * -direction.z;
		}
	}
	else
	{
		if (tMax.y < tMax.z)
		{
			res.y += direction.y;
			newT = tMax.y;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[2] * -direction.y;
		}
		else
		{
			res.z += direction.z;
			newT = tMax.z;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[4] * -direction.z;
		}
	}

	return res;
}



float3 GetPositionAlongRay(in Ray ray, in float t)
{
	return ray.origin + ray.direction * t;
}

bool IsValidVoxelIndex(in int3 index)
{
	return index.x >= 0 && index.x < g_sceneCB.voxelAxisCount && index.y >= 0 && index.y < g_sceneCB.voxelAxisCount && index.z >= 0 && index.z < g_sceneCB.voxelAxisCount;
}

bool IsSolid(in int3 index)
{
	return g_voxelVolume[index].r > 0;
}

inline float4 GetCellVoxel1(in int3 cellOriginIndex)
{
	return g_voxelVolume[cellOriginIndex];
}

inline float4 GetCellVoxel2(in int3 cellOriginIndex)
{
	return	g_voxelVolume[int3(cellOriginIndex.x + 1, cellOriginIndex.y, cellOriginIndex.z)];
}

inline float4 GetCellVoxel3(in int3 cellOriginIndex)
{
	return	g_voxelVolume[int3(cellOriginIndex.x + 1, cellOriginIndex.y + 1, cellOriginIndex.z)];
}

inline float4 GetCellVoxel4(in int3 cellOriginIndex)
{
	return	g_voxelVolume[int3(cellOriginIndex.x, cellOriginIndex.y + 1, cellOriginIndex.z)];
}

inline float4 GetCellVoxel5(in int3 cellOriginIndex)
{
	return g_voxelVolume[int3(cellOriginIndex.x, cellOriginIndex.y, cellOriginIndex.z + 1)];
}

inline float4 GetCellVoxel6(in int3 cellOriginIndex)
{
	return	g_voxelVolume[int3(cellOriginIndex.x + 1, cellOriginIndex.y, cellOriginIndex.z + 1)];
}

inline float4 GetCellVoxel7(in int3 cellOriginIndex)
{
	return	g_voxelVolume[int3(cellOriginIndex.x + 1, cellOriginIndex.y + 1, cellOriginIndex.z + 1)];
}

inline float4 GetCellVoxel8(in int3 cellOriginIndex)
{
	return	g_voxelVolume[int3(cellOriginIndex.x, cellOriginIndex.y + 1, cellOriginIndex.z + 1)];
}

bool HasIsoSurfaceInsideCell(in int3 cellOriginIndex)
{
	float4 v1 = GetCellVoxel1(cellOriginIndex);
	float4 v2 = GetCellVoxel2(cellOriginIndex);
	float4 v3 = GetCellVoxel3(cellOriginIndex);
	float4 v4 = GetCellVoxel4(cellOriginIndex);
	float4 v5 = GetCellVoxel5(cellOriginIndex);
	float4 v6 = GetCellVoxel6(cellOriginIndex);
	float4 v7 = GetCellVoxel7(cellOriginIndex);
	float4 v8 = GetCellVoxel8(cellOriginIndex);

	//return (sign(v1.g) & sign(v2.g) & sign(v3.g) & sign(v4.g) & sign(v5.g) & sign(v6.g) & sign(v7.g) & sign(v8.g)) != sign(v1.g);

	return  v1.g != v2.g ||
			v1.g != v3.g ||
			v1.g != v4.g ||
			v1.g != v5.g ||
			v1.g != v6.g ||
			v1.g != v7.g ||
			v1.g != v8.g;
}

float GetDensity(in int3 cellOriginIndex, in float3 normPosInsideCell)
{
	float4 v1 = GetCellVoxel1(cellOriginIndex);
	float4 v2 = GetCellVoxel2(cellOriginIndex);
	float4 v3 = GetCellVoxel3(cellOriginIndex);
	float4 v4 = GetCellVoxel4(cellOriginIndex);
	float4 v5 = GetCellVoxel5(cellOriginIndex);
	float4 v6 = GetCellVoxel6(cellOriginIndex);
	float4 v7 = GetCellVoxel7(cellOriginIndex);
	float4 v8 = GetCellVoxel8(cellOriginIndex);

	float vx1 = lerp(v1.g, v2.g, normPosInsideCell.x); 
	float vx2 = lerp(v4.g, v3.g, normPosInsideCell.x); 
	float vx3 = lerp(v5.g, v6.g, normPosInsideCell.x); 
	float vx4 = lerp(v8.g, v7.g, normPosInsideCell.x);
	
	float vy1 = lerp(vx1, vx2, normPosInsideCell.y);
	float vy2 = lerp(vx3, vx4, normPosInsideCell.y);

	float vz = lerp(vy1, vy2, normPosInsideCell.z);

	return vz;
}

float3 GetVoxelNormal(in int3 cellIndex, in float3 normVoxelPos)
{
	const float accuracy = 0.1;

	float nX = GetDensity(cellIndex, normVoxelPos + float3(accuracy, 0, 0)) - GetDensity(cellIndex, normVoxelPos - float3(accuracy, 0, 0));
	float nY = GetDensity(cellIndex, normVoxelPos + float3(0, accuracy, 0)) - GetDensity(cellIndex, normVoxelPos - float3(0, accuracy, 0));
	float nZ = GetDensity(cellIndex, normVoxelPos + float3(0, 0, accuracy)) - GetDensity(cellIndex, normVoxelPos - float3(0, 0, accuracy));

	//return normalize(float3(nX, nY, nZ));
	return float3(nX, nY, nZ);
}

float3 GetNormal(in int3 cellIndex, in float3 normPosInsideCell)
{
	float3 n1 = GetVoxelNormal(cellIndex, float3(0,0,0));
	float3 n2 = GetVoxelNormal(cellIndex, float3(1,0,0));
	float3 n3 = GetVoxelNormal(cellIndex, float3(1,1,0));
	float3 n4 = GetVoxelNormal(cellIndex, float3(0,1,0));
	float3 n5 = GetVoxelNormal(cellIndex, float3(0,0,1));
	float3 n6 = GetVoxelNormal(cellIndex, float3(1,0,1));
	float3 n7 = GetVoxelNormal(cellIndex, float3(1,1,1));
	float3 n8 = GetVoxelNormal(cellIndex, float3(0,1,1));

	float3 nx1 = lerp(n1, n2, normPosInsideCell.x);
	float3 nx2 = lerp(n4, n3, normPosInsideCell.x);
	float3 nx3 = lerp(n5, n6, normPosInsideCell.x);
	float3 nx4 = lerp(n8, n7, normPosInsideCell.x);

	float3 ny1 = lerp(nx1, nx2, normPosInsideCell.y);
	float3 ny2 = lerp(nx3, nx4, normPosInsideCell.y);

	float3 nz = lerp(ny1, ny2, normPosInsideCell.z);

	return n1;
}

[shader("raygeneration")]
void VRRaygen()
{
	Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, g_sceneCB.cameraPosition.xyz, g_sceneCB.viewMatrixInverted, g_sceneCB.projectionMatrixInverted);

	uint currentRecursionDepth = 0;
	float4 color = TraceRadianceRay(ray, currentRecursionDepth);

	g_renderTarget[DispatchRaysIndex().xy] = color;
}

[shader("closesthit")]
void VRClosestHit(inout VolumeRaytracer::VRayPayload rayPayload, in VolumeRaytracer::VPrimitiveAttributes attr)
{
	rayPayload.color = float4(attr.normal, 1);
	//rayPayload.color = float4(abs((attr.normal + 1) * 0.5), 1);
}

[shader("intersection")]
void VRIntersection()
{
	Ray ray;

	ray.origin = WorldRayOrigin();
	ray.direction = WorldRayDirection();

	//Check if we intersect voxel volume
	float3 volumeAABB[2] = {float3(-g_sceneCB.volumeExtend, -g_sceneCB.volumeExtend, -g_sceneCB.volumeExtend), float3(g_sceneCB.volumeExtend, g_sceneCB.volumeExtend, g_sceneCB.volumeExtend)};
	float tEnter, tExit;

	float tMax = RayTCurrent();

	if (DetermineRayAABBIntersection(ray, volumeAABB, tEnter, tExit))
	{
		int3 voxelDir = sign(ray.direction);
		int3 voxelPos;
		int3 nextVoxelPos;
		float3 hitNormal = -ray.direction;
		float cellExit;
		float cellEnter;
		float tHit;

		tEnter += 0.0001;

		if (tEnter >= 0)
		{
			voxelPos = WorldSpaceToVoxelSpace(GetPositionAlongRay(ray, tEnter));
		}
		else
		{
			voxelPos = WorldSpaceToVoxelSpace(ray.origin);
		}

		cellExit = tEnter;

		int maxIterations = 3000;

		while (cellExit <= tMax && maxIterations > 0)
		{
			maxIterations--;

			cellEnter = cellExit;

			nextVoxelPos = GoToNextVoxel(ray, voxelPos, voxelDir, cellExit, hitNormal);

			if (IsValidCell(voxelPos))
			{
				//if (GetDensity(int3(0, 0, 0), float3(1, 1, 1)) < 0)
				//{
				//	VolumeRaytracer::VPrimitiveAttributes attr;
				//	attr.normal = voxelPos;

				//	ReportHit(cellEnter, 0, attr);
				//}

				if (HasIsoSurfaceInsideCell(voxelPos))
				{
					float3 enterCellSpace = WorldSpaceToCellSpace(voxelPos, GetPositionAlongRay(ray, cellEnter));
					float3 exitCellSpace = WorldSpaceToCellSpace(voxelPos, GetPositionAlongRay(ray, cellExit));

					float densityEnter = GetDensity(voxelPos, enterCellSpace);

					if (densityEnter < 0)
					{
						VolumeRaytracer::VPrimitiveAttributes attr;
						attr.normal = GetNormal(voxelPos, WorldSpaceToCellSpace(voxelPos, GetPositionAlongRay(ray, cellEnter)));;

						ReportHit(cellEnter, 0, attr);

						break;
					}

					float densityExit = GetDensity(voxelPos, exitCellSpace);

					//if (densityExit > 0)
					//{
					//	VolumeRaytracer::VPrimitiveAttributes attr;
					//	attr.normal = float3(1, 0, 0);

					//	ReportHit(tEnter, 0, attr);

					//	break;
					//}

					if (sign(densityEnter) != sign(densityExit))
					{
						float k = densityExit - densityEnter;
						float interpA = -densityExit / k;

						tHit = lerp(cellEnter, cellExit, (-densityExit / k));

						VolumeRaytracer::VPrimitiveAttributes attr;
						attr.normal = GetNormal(voxelPos, WorldSpaceToCellSpace(voxelPos, GetPositionAlongRay(ray, tHit)));

						ReportHit(tHit, 0, attr);

						break;
					}
				}
			}
			
			voxelPos = nextVoxelPos;
		}

		//if (maxIterations <= 0)
		//{
		//	VolumeRaytracer::VPrimitiveAttributes attr;
		//	attr.normal = float3(1, 0, 0);

		//	ReportHit(tEnter, 0, attr);
		//}
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