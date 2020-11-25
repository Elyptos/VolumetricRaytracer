/*
	Copyright (c) 2020 Thomas Sch�ngrundner

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
Texture3D<float> g_voxelVolume : register(t1, space0);

TextureCube g_envMap : register(t2, space0);
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
	rayDesc.TMax = 10000;

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

uint3 WorldSpaceToVoxelSpace(in float3 worldLocation)
{
	float3 relativeVolumePos = worldLocation - g_sceneCB.volumeExtend;
	float distanceBetweenVoxel = (g_sceneCB.volumeExtend * 2) / g_sceneCB.voxelAxisCount;

	return uint3(relativeVolumePos.x / distanceBetweenVoxel, relativeVolumePos.y / distanceBetweenVoxel, relativeVolumePos.z / distanceBetweenVoxel);
}

float3 VoxelIndexToWorldSpace(in uint3 voxelIndex)
{
	float distanceBetweenVoxel = (g_sceneCB.volumeExtend * 2) / g_sceneCB.voxelAxisCount;
	float3 voxelIndexF = voxelIndex;

	return voxelIndexF * distanceBetweenVoxel + distanceBetweenVoxel * 0.5f - g_sceneCB.volumeExtend;
}

uint3 GoToNextVoxel(in Ray ray, in float currentT, in uint3 voxelIndex, in int3 direction, out float newT, out float3 voxelFaceNormal)
{
	float maxX, maxY, maxZ = 100000;
	float3 voxelMax = VoxelIndexToWorldSpace(voxelIndex) + ((g_sceneCB.volumeExtend * 2) / g_sceneCB.voxelAxisCount) * 0.5f;
	uint3 res = voxelIndex;

	if (ray.direction.x != 0)
	{
		maxX = (voxelMax.x - ray.origin.x) / ray.direction.x;
	}

	if (ray.direction.y != 0)
	{
		maxY = (voxelMax.y - ray.origin.y) / ray.direction.y;
	}

	if (ray.direction.z != 0)
	{
		maxZ = (voxelMax.z - ray.origin.z) / ray.direction.z;
	}

	if (maxX < maxY)
	{
		if (maxX < maxZ)
		{
			res.x += direction.x;
			newT = maxX;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[0] * -direction.x;
		}
		else
		{
			res.z += direction.z;
			newT = maxZ;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[4] * -direction.z;
		}
	}
	else
	{
		if (maxY < maxZ)
		{
			res.y += direction.y;
			newT = maxY;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[2] * -direction.y;
		}
		else
		{
			res.z += direction.z;
			newT = maxZ;
			voxelFaceNormal = VolumeRaytracer::CubeNormals[4] * -direction.z;
		}
	}

	return res;
}



float3 GetPositionAlongRay(in Ray ray, in float t)
{
	return ray.origin + ray.direction * t;
}

bool IsValidVoxelIndex(in uint3 index)
{
	return index.x < g_sceneCB.voxelAxisCount && index.y < g_sceneCB.voxelAxisCount && index.z < g_sceneCB.voxelAxisCount;
}

bool IsSolid(in uint3 index)
{
	return g_voxelVolume[index].r > 0;
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
	rayPayload.color = float4(abs((attr.normal + 1) * 0.5), 1);
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

	if (DetermineRayAABBIntersection(ray, volumeAABB, tEnter, tExit))
	{
		/*int3 voxelDir = sign(ray.direction);
		uint3 voxelPos;
		float3 hitNormal = -ray.direction;

		if (tEnter >= 0)
		{
			voxelPos = WorldSpaceToVoxelSpace(GetPositionAlongRay(ray, tEnter));
		}
		else
		{
			voxelPos = WorldSpaceToVoxelSpace(ray.origin);
		}

		tExit = tEnter;

		while (IsValidVoxelIndex(voxelPos) && !IsSolid(voxelPos))
		{
			tEnter = tExit;
			voxelPos = GoToNextVoxel(ray, tEnter, voxelPos, voxelDir, tExit, hitNormal);
		}

		if (IsValidVoxelIndex(voxelPos) && IsSolid(voxelPos))
		{
			VolumeRaytracer::VPrimitiveAttributes attr;
			attr.normal = hitNormal;

			ReportHit(tEnter, 0, attr);
		}*/

		VolumeRaytracer::VPrimitiveAttributes attr;
		attr.normal = float3(0, 1, 0);

		ReportHit(tEnter, 0, attr);
	}
	//else
	//{
	//	VolumeRaytracer::VPrimitiveAttributes attr;
	//	attr.normal = float3(1, 0, 0);

	//	ReportHit(RayTCurrent(), 0, attr);
	//}

	//VolumeRaytracer::VPrimitiveAttributes attr;
	//attr.normal = float3(1, 0, 0);

	//ReportHit(RayTCurrent(), 0, attr);
}

[shader("miss")]
void VRMiss(inout VolumeRaytracer::VRayPayload rayPayload)
{
	float3 rayDirection = WorldRayDirection();
	/*float3 worldX = float3(1,0,0);
	float yawCos = dot(rayDirection, worldX);

	rayPayload.color = float4(abs(yawCos), 0, 0, 1);*/

	rayPayload.color = g_envMap.SampleLevel(g_envMapSampler, rayDirection.xzy, 0);
}

[shader("miss")]
void VRMissShadowRay(inout VolumeRaytracer::VShadowRayPayload rayPayload)
{
	rayPayload.hit = false;
}