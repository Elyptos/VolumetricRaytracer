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
RWTexture2D<float4> g_renderTarget : register(u0);
ConstantBuffer<VolumeRaytracer::VSceneConstantBuffer> g_sceneCB : register(b0);

struct Ray
{
	float3 origin;
	float3 direction;
};

inline Ray GenerateRay(uint2 index, in float3 cameraPosition, in float4x4 projectionToWorld)
{
	//Calculate screen space coordinates between -1 and 1 so that 0 is in the center of the screen
	uint2 renderTargetSize = DispatchRaysDimensions().xy;
	float2 xy = index + 0.5f;
	float2 screenPos = xy / renderTargetSize * 2.0 - 1.0;

	screenPos.y = -screenPos.y;

	//Convert to world position
	float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);
	world.xyz /= world.w;

	Ray ray;
	ray.origin = cameraPosition;
	ray.direction = normalize(world.xyz - ray.origin);

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

[shader("raygeneration")]
void VRRaygen()
{
	float3 cameraPos = g_sceneCB.cameraPosition.xyz;
	float4x4 projectionToWorld = g_sceneCB.projectionToWorld;
	uint2 currentRayIndex = DispatchRaysIndex().xy;

	Ray ray = GenerateRay(currentRayIndex, cameraPos, projectionToWorld);

	uint currentRecursionDepth = 0;
	float4 color = TraceRadianceRay(ray, currentRecursionDepth);

	g_renderTarget[currentRayIndex] = color;
}

[shader("closesthit")]
void VRClosestHit(inout VolumeRaytracer::VRayPayload rayPayload, in VolumeRaytracer::VPrimitiveAttributes attr)
{
	
}

[shader("intersection")]
void VRIntersection()
{

}

[shader("miss")]
void VRMiss(inout VolumeRaytracer::VRayPayload rayPayload)
{
	float4 backgroundColor = float4(VolumeRaytracer::BackgroundColor);
	rayPayload.color = backgroundColor;
}

[shader("miss")]
void VRMissShadowRay(inout VolumeRaytracer::VShadowRayPayload rayPayload)
{
	rayPayload.hit = false;
}