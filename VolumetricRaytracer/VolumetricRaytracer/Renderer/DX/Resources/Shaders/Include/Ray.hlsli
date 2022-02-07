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