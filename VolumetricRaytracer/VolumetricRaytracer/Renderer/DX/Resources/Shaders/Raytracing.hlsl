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

Texture3D<float> g_voxelVolume[] : register(t2, space0);
ConstantBuffer<VolumeRaytracer::VGeometryConstantBuffer> g_geometryCB[] : register(b1);

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

inline Ray GetLocalRay()
{
	Ray ray;

	ray.origin = ObjectRayOrigin();
	ray.direction = ObjectRayDirection();

	return ray;
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

bool TraceShadowRay(in Ray ray, in uint currentRayRecursionDepth)
{
	if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
	{
		return false;
	}

	RayDesc rayDesc;
	rayDesc.Origin = ray.origin;
	rayDesc.Direction = ray.direction;

	rayDesc.TMin = 0;
	rayDesc.TMax = 600;

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

inline float3 WorldSpaceToCellSpace(in int3 cellOriginPos, in float3 worldLocation)
{
	uint instance = InstanceID();

	float3 voxelOriginPos = VoxelIndexToWorldSpace(cellOriginPos);
	float3 rel = worldLocation - voxelOriginPos;

	return rel / g_geometryCB[instance].distanceBtwVoxels;
}

float3 WorldSpaceToBottomLevelCellSpace(in int3 cellOrigin, in float3 worldSpace)
{
	uint instance = InstanceID();
	float3 voxelPos = VoxelIndexToWorldSpace(cellOrigin);

	return (worldSpace - voxelPos) / g_geometryCB[instance].distanceBtwVoxels;
}

float3 WorldSpaceToTopLevelCellSpace(in int3 cellOrigin, in float3 worldSpace)
{
	uint instance = InstanceID();
	float3 voxelPos = VoxelIndexToWorldSpace(cellOrigin + int3(1, 1, 1));

	return abs(worldSpace - voxelPos) / g_geometryCB[instance].distanceBtwVoxels;
}

float3 WorldDirectionToBottomLevelCellSpace(in float3 worldDirection)
{
	uint instance = InstanceID();
	return worldDirection / g_geometryCB[instance].distanceBtwVoxels;
}

float3 WorldDirectionToTopLevelCellSpace(in float3 worldDirection)
{
	uint instance = InstanceID();
	return -worldDirection / g_geometryCB[instance].distanceBtwVoxels;
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

int3 GoToNextVoxel(in Ray ray, in uint3 voxelIndex, in int3 direction, out float newT, out float3 voxelFaceNormal)
{
	uint instance = InstanceID();
	float3 tMax = float3(100000, 100000, 100000);
	int3 res = voxelIndex;
	const float FLT_INFINITY = 1.#INF;
	float3 invRayDirection = ray.direction != 0 ? 1 / ray.direction : (ray.direction > 0) ? FLT_INFINITY : -FLT_INFINITY;

	int3 sign = ray.direction > 0;

	float3 voxelPos = VoxelIndexToWorldSpace(voxelIndex);
	float3 voxelExtends = g_geometryCB[instance].distanceBtwVoxels * 0.5;
	float3 aabb[2] = { voxelPos, voxelPos + g_geometryCB[instance].distanceBtwVoxels };

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
	uint instance = InstanceID();
	return index.x >= 0 && index.x < g_geometryCB[instance].voxelAxisCount && index.y >= 0 && index.y < g_geometryCB[instance].voxelAxisCount && index.z >= 0 && index.z < g_geometryCB[instance].voxelAxisCount;
}

bool IsSolid(in int3 index)
{
	uint instance = InstanceID();
	return g_voxelVolume[instance][index] <= 0;
}

inline float GetCellVoxel1(in int3 cellOriginIndex)
{
	uint instance = InstanceID();
	return g_voxelVolume[instance][cellOriginIndex];
}

inline float GetCellVoxel2(in int3 cellOriginIndex)
{
	uint instance = InstanceID();
	return	g_voxelVolume[instance][int3(cellOriginIndex.x + 1, cellOriginIndex.y, cellOriginIndex.z)];
}

inline float GetCellVoxel3(in int3 cellOriginIndex)
{
	uint instance = InstanceID();
	return	g_voxelVolume[instance][int3(cellOriginIndex.x + 1, cellOriginIndex.y + 1, cellOriginIndex.z)];
}

inline float GetCellVoxel4(in int3 cellOriginIndex)
{
	uint instance = InstanceID();
	return	g_voxelVolume[instance][int3(cellOriginIndex.x, cellOriginIndex.y + 1, cellOriginIndex.z)];
}

inline float GetCellVoxel5(in int3 cellOriginIndex)
{
	uint instance = InstanceID();
	return g_voxelVolume[instance][int3(cellOriginIndex.x, cellOriginIndex.y, cellOriginIndex.z + 1)];
}

inline float GetCellVoxel6(in int3 cellOriginIndex)
{
	uint instance = InstanceID();
	return	g_voxelVolume[instance][int3(cellOriginIndex.x + 1, cellOriginIndex.y, cellOriginIndex.z + 1)];
}

inline float GetCellVoxel7(in int3 cellOriginIndex)
{
	uint instance = InstanceID();
	return	g_voxelVolume[instance][int3(cellOriginIndex.x + 1, cellOriginIndex.y + 1, cellOriginIndex.z + 1)];
}

inline float GetCellVoxel8(in int3 cellOriginIndex)
{
	uint instance = InstanceID();
	return	g_voxelVolume[instance][int3(cellOriginIndex.x, cellOriginIndex.y + 1, cellOriginIndex.z + 1)];
}

bool HasIsoSurfaceInsideCell(in int3 cellOriginIndex)
{
	float v1 = GetCellVoxel1(cellOriginIndex);
	float v2 = GetCellVoxel2(cellOriginIndex);
	float v3 = GetCellVoxel3(cellOriginIndex);
	float v4 = GetCellVoxel4(cellOriginIndex);
	float v5 = GetCellVoxel5(cellOriginIndex);
	float v6 = GetCellVoxel6(cellOriginIndex);
	float v7 = GetCellVoxel7(cellOriginIndex);
	float v8 = GetCellVoxel8(cellOriginIndex);

	float v1Sign = sign(v1);

	return  v1Sign != sign(v2) ||
			v1Sign != sign(v3) ||
			v1Sign != sign(v4) ||
			v1Sign != sign(v5) ||
			v1Sign != sign(v6) ||
			v1Sign != sign(v7) ||
			v1Sign != sign(v8);
}

bool IsSolidCell(in int3 cellOriginIndex)
{
	float v1 = GetCellVoxel1(cellOriginIndex);
	float v2 = GetCellVoxel2(cellOriginIndex);
	float v3 = GetCellVoxel3(cellOriginIndex);
	float v4 = GetCellVoxel4(cellOriginIndex);
	float v5 = GetCellVoxel5(cellOriginIndex);
	float v6 = GetCellVoxel6(cellOriginIndex);
	float v7 = GetCellVoxel7(cellOriginIndex);
	float v8 = GetCellVoxel8(cellOriginIndex);

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

void GetDensityPolynomial(in Ray ray, in int3 cellIndex, in float tIn, in float tOut, out float A, out float B, out float C, out float D)
{
	uint instance = InstanceID();
	float3 a1 = WorldSpaceToBottomLevelCellSpace(cellIndex, GetPositionAlongRay(ray, tIn));
	float3 a0 = 1 - a1;
	float3 b1 = WorldSpaceToBottomLevelCellSpace(cellIndex, GetPositionAlongRay(ray, tOut)) - a1;
	float3 b0 = -b1;

	int3 v000 = cellIndex;
	int3 v100 = cellIndex + int3(1, 0, 0);
	int3 v010 = cellIndex + int3(0, 1, 0);
	int3 v110 = cellIndex + int3(1, 1, 0);
	int3 v001 = cellIndex + int3(0, 0, 1);
	int3 v101 = cellIndex + int3(1, 0, 1);
	int3 v111 = cellIndex + int3(1, 1, 1);
	int3 v011 = cellIndex + int3(0, 1, 1);

	A =	b0.x * b0.y * b0.z * g_voxelVolume[instance][v000] + 
		b1.x * b0.y * b0.z * g_voxelVolume[instance][v100] +
		b0.x * b1.y * b0.z * g_voxelVolume[instance][v010] +
		b1.x * b1.y * b0.z * g_voxelVolume[instance][v110] +
		b0.x * b0.y * b1.z * g_voxelVolume[instance][v001] +
		b1.x * b0.y * b1.z * g_voxelVolume[instance][v101] +
		b0.x * b1.y * b1.z * g_voxelVolume[instance][v011] +
		b1.x * b1.y * b1.z * g_voxelVolume[instance][v111];

	D = a0.x * a0.y * a0.z * g_voxelVolume[instance][v000] +
		a1.x * a0.y * a0.z * g_voxelVolume[instance][v100] +
		a0.x * a1.y * a0.z * g_voxelVolume[instance][v010] +
		a1.x * a1.y * a0.z * g_voxelVolume[instance][v110] +
		a0.x * a0.y * a1.z * g_voxelVolume[instance][v001] +
		a1.x * a0.y * a1.z * g_voxelVolume[instance][v101] +
		a0.x * a1.y * a1.z * g_voxelVolume[instance][v011] +
		a1.x * a1.y * a1.z * g_voxelVolume[instance][v111];

	B = (a0.x * b0.y * b0.z + b0.x * a0.y * b0.z + b0.x * b0.y * a0.z) * g_voxelVolume[instance][v000] +
		(a1.x * b0.y * b0.z + b1.x * a0.y * b0.z + b1.x * b0.y * a0.z) * g_voxelVolume[instance][v100] +
		(a0.x * b1.y * b0.z + b0.x * a1.y * b0.z + b0.x * b1.y * a0.z) * g_voxelVolume[instance][v010] +
		(a1.x * b1.y * b0.z + b1.x * a1.y * b0.z + b1.x * b1.y * a0.z) * g_voxelVolume[instance][v110] +
		(a0.x * b0.y * b1.z + b0.x * a0.y * b1.z + b0.x * b0.y * a1.z) * g_voxelVolume[instance][v001] +
		(a1.x * b0.y * b1.z + b1.x * a0.y * b1.z + b1.x * b0.y * a1.z) * g_voxelVolume[instance][v101] +
		(a0.x * b1.y * b1.z + b0.x * a1.y * b1.z + b0.x * b1.y * a1.z) * g_voxelVolume[instance][v011] +
		(a1.x * b1.y * b1.z + b1.x * a1.y * b1.z + b1.x * b1.y * a1.z) * g_voxelVolume[instance][v111];

	C = (b0.x * a0.y * a0.z + a0.x * b0.y * a0.z + a0.x * a0.y * b0.z) * g_voxelVolume[instance][v000] +
		(b1.x * a0.y * a0.z + a1.x * b0.y * a0.z + a1.x * a0.y * b0.z) * g_voxelVolume[instance][v100] +
		(b0.x * a1.y * a0.z + a0.x * b1.y * a0.z + a0.x * a1.y * b0.z) * g_voxelVolume[instance][v010] +
		(b1.x * a1.y * a0.z + a1.x * b1.y * a0.z + a1.x * a1.y * b0.z) * g_voxelVolume[instance][v110] +
		(b0.x * a0.y * a1.z + a0.x * b0.y * a1.z + a0.x * a0.y * b1.z) * g_voxelVolume[instance][v001] +
		(b1.x * a0.y * a1.z + a1.x * b0.y * a1.z + a1.x * a0.y * b1.z) * g_voxelVolume[instance][v101] +
		(b0.x * a1.y * a1.z + a0.x * b1.y * a1.z + a0.x * a1.y * b1.z) * g_voxelVolume[instance][v011] +
		(b1.x * a1.y * a1.z + a1.x * b1.y * a1.z + a1.x * a1.y * b1.z) * g_voxelVolume[instance][v111];
}

float GetDensity(in int3 cellOriginIndex, in float3 cellPos)
{
	if(!IsValidCell(cellOriginIndex))
		return -1.f;

	uint instance = InstanceID();

	float p = 0.f;

	for (int i = 0; i <= 1; i++)
	{
		for (int j = 0; j <= 1; j++)
		{
			for (int k = 0; k <= 1; k++)
			{
				float u = abs((1 - i) - cellPos.x);
				float v = abs((1 - j) - cellPos.y);
				float w = abs((1 - k) - cellPos.z);

				p += (u * v * w * g_voxelVolume[instance][cellOriginIndex + int3(i, j, k)]);
			}
		}
	}

	return p;
}

inline float GetDensityWithPolynomial(in float t, in float A, in float B, in float C, in float D)
{
	return A*t*t*t + B*t*t + C*t + D;
}

bool GetSurfaceIntersectionT(in Ray ray, in int3 cellIndex, in float tIn, in float tOut, out float tHit)
{
	float A, B, C, D = 0;
	float t0 = max(0, -tIn / (tOut - tIn));
	float t1 = 1;

	GetDensityPolynomial(ray, cellIndex, tIn, tOut, A, B, C, D);

	if (A == 0)
	{
		return false;
	}

	float dA = 3 * A;
	float dB = 2 * B;

	float ex1 = (-dB + sqrt(dB*dB - 4*dA*C)) / (2 * dA);
	float ex2 = (-dB - sqrt(dB*dB - 4*dA*C)) / (2 * dA);

	float f0 = GetDensityWithPolynomial(t0, A, B, C, D);
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

	float x = (GetDensity(cellIndex + int3(1,0,0), normPos) - GetDensity(cellIndex - int3(1,0,0), normPos)) / (2 * g_geometryCB[instance].distanceBtwVoxels);
	float y = (GetDensity(cellIndex + int3(0,1,0), normPos) - GetDensity(cellIndex - int3(0,1,0), normPos)) / (2 * g_geometryCB[instance].distanceBtwVoxels);
	float z = (GetDensity(cellIndex + int3(0,0,1), normPos) - GetDensity(cellIndex - int3(0,0,1), normPos)) / (2 * g_geometryCB[instance].distanceBtwVoxels);

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

[shader("closesthit")]
void VRClosestHit(inout VolumeRaytracer::VRayPayload rayPayload, in VolumeRaytracer::VPrimitiveAttributes attr)
{
	float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
	float3 shadowRayOrigin = hitPosition + g_sceneCB.dirLightDirection * 0.01;

	Ray shadowRay;
	shadowRay.origin = shadowRayOrigin;
	shadowRay.direction = g_sceneCB.dirLightDirection;

	bool shadowRayHit = TraceShadowRay(shadowRay, rayPayload.depth);

	float diffuse = (0.5 / PI) * g_sceneCB.dirLightStrength * dot(attr.normal, g_sceneCB.dirLightDirection);
	//float diffuse = 1.f;

	diffuse *= (shadowRayHit ? 0.2 : 1);

	//rayPayload.color.rgb = float3(1.f, 1.f, 1.f) * diffuse;
	rayPayload.color.rgb = attr.normal;
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
		int3 voxelDir = sign(localRay.direction);
		int3 voxelPos;
		int3 nextVoxelPos;
		float3 hitNormal = -localRay.direction;
		float cellExit;
		float cellEnter;
		float tHit;

		if (tEnter >= 0)
		{
			tEnter += 0.01;

			voxelPos = WorldSpaceToVoxelSpace(GetPositionAlongRay(localRay, tEnter));
			cellExit = tEnter;
		}
		else
		{
			voxelPos = WorldSpaceToVoxelSpace(localRay.origin);

			//Cell exit is behind the ray origin, to calculate it we traverse to the previous voxel.
			GoToNextVoxel(ReverseRay(localRay), voxelPos, -voxelDir, cellExit, hitNormal);

			cellExit = -cellExit;
			cellExit += 0.01;
		}

		int maxIterations = 3000;

		if (IsValidCell(voxelPos) && IsSolidCell(voxelPos))
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

			nextVoxelPos = GoToNextVoxel(localRay, voxelPos, voxelDir, cellExit, hitNormal);

			if (IsValidCell(voxelPos))
			{
				//VolumeRaytracer::VPrimitiveAttributes attr;
				//attr.normal = float3(1, 1, 1);

				//ReportHit(10, 0, attr);

				//return;

				if (HasIsoSurfaceInsideCell(voxelPos))
				{
					if (GetSurfaceIntersectionT(localRay, voxelPos, cellEnter, cellExit, tHit))
					{
						VolumeRaytracer::VPrimitiveAttributes attr;
						attr.normal = GetNormal(voxelPos, WorldSpaceToBottomLevelCellSpace(voxelPos, GetPositionAlongRay(localRay, tHit)));

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
			
			voxelPos = nextVoxelPos;
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