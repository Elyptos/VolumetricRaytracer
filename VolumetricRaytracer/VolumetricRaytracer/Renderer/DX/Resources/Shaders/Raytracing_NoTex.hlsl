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
//#define SHADER_DEBUG

#include "RaytracingHlsl.h"
#include "Include/Parameters.hlsli"
#include "Include/Lighting.hlsli"
#include "Include/Quaternion.hlsli"
#include "Include/Ray.hlsli"
#include "Include/Voxel.hlsli"
#include "Include/Textures.hlsli"

[shader("raygeneration")]
void VRRaygen()
{
	Ray ray = GenerateCameraRay(DispatchRaysIndex().xy, g_sceneCB.cameraPosition.xyz, g_sceneCB.viewMatrixInverted, g_sceneCB.projectionMatrixInverted);

	uint currentRecursionDepth = 0;
	float4 color = TraceRadianceRay(ray, currentRecursionDepth);
	
	float3 colorRGB = color.rgb;
	colorRGB = colorRGB / (colorRGB + 1.0f);
	colorRGB = pow(colorRGB, 1.0f / 2.2);
	
	g_renderTarget[DispatchRaysIndex().xy] = float4(colorRGB, 1.0f);
}

[shader("closesthit")]
void VRClosestHit(inout VolumeRaytracer::VRayPayload rayPayload, in VolumeRaytracer::VPrimitiveAttributes attr)
{
	if (attr.unlit)
	{
		rayPayload.color.rgb = attr.normal;
		rayPayload.color.a = 1.f;
	}
	else
	{
		float3 hitPosition = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
		float3 shadowRayOrigin = hitPosition - WorldRayDirection() * 0.1f;

		//Trace for directional light
		Ray shadowRay;
		shadowRay.origin = shadowRayOrigin;
		shadowRay.direction = g_sceneCB.dirLightDirection;

		bool shadowRayHit = TraceShadowRay(shadowRay, rayPayload.depth, 5000.f);
	
		float3 diffuse = float3(SHADOW_BRIGHTNESS, SHADOW_BRIGHTNESS, SHADOW_BRIGHTNESS);
		float3 Li = float3(g_sceneCB.dirLightStrength, g_sceneCB.dirLightStrength, g_sceneCB.dirLightStrength);
		
		float3 albedo = g_geometryCB[InstanceID()].tint.rgb;
		float k = g_geometryCB[InstanceID()].k;
		
		float roughness = clamp(g_geometryCB[InstanceID()].roughness, 0.0f, 1.0f);
		float metallness = clamp(g_geometryCB[InstanceID()].metallness, 0.0f, 1.0f);
		float3 normal = attr.normal;

		float4 norm4 = float4(normal, 0.f);
		
		normal = mul(norm4, ObjectToWorld4x3()).xyz;
		
		float3 reflactanceColor = float3(0, 0, 0);
		
		if (roughness < 0.3f)
		{
			Ray reflectionRay;
			reflectionRay.origin = shadowRayOrigin;
			reflectionRay.direction = normalize(WorldRayDirection() - 2 * dot(WorldRayDirection(), normal) * normal);
			
			reflactanceColor = TraceRadianceRay(reflectionRay, rayPayload.depth).rgb;
			reflactanceColor = max(float3(0, 0, 0), lerp(reflactanceColor, float3(0, 0, 0), roughness * 2.2f));
			
			diffuse += Radiance(reflactanceColor, reflectionRay.direction, -WorldRayDirection(), normal, albedo, roughness, metallness, k);
		}
		
		if (!shadowRayHit)
		{
			diffuse += Radiance(Li, g_sceneCB.dirLightDirection, -WorldRayDirection(), normal, albedo, roughness, metallness, k);
			
			//diffuse += (0.5 / PI) * g_sceneCB.dirLightStrength * dot(attr.normal, g_sceneCB.dirLightDirection);
		}

		float distanceToLightSource = 0;
		float lightIntensity = 0;
	
		for (int pi = 0; pi < g_sceneCB.numPointLights; pi++)
		{
			distanceToLightSource = length(g_pointLightsCB[pi].position - shadowRayOrigin);
			lightIntensity = ComputePointLightIntensity(g_pointLightsCB[pi].lightIntensity, distanceToLightSource, g_pointLightsCB[pi].attLinear, g_pointLightsCB[pi].attExp);
		
			Li = g_pointLightsCB[pi].color.rgb * lightIntensity;
			
			if (lightIntensity > 0.005f)
			{
				shadowRay.direction = normalize(g_pointLightsCB[pi].position - shadowRayOrigin);
				shadowRayHit = TraceShadowRay(shadowRay, rayPayload.depth, distanceToLightSource);
		
				if (!shadowRayHit)
				{
					diffuse += Radiance(Li, shadowRay.direction, -WorldRayDirection(), normal, albedo, roughness, metallness, k);
				}
			}
		}
	
		for (int si = 0; si < g_sceneCB.numSpotLights; si++)
		{
			distanceToLightSource = length(g_spotLightsCB[si].position - shadowRayOrigin);
			lightIntensity = ComputeSpotLightIntensity(shadowRayOrigin, distanceToLightSource, g_spotLightsCB[si].position, g_spotLightsCB[si].forward, g_spotLightsCB[si].lightIntensity, g_spotLightsCB[si].attLinear, g_spotLightsCB[si].attExp, g_spotLightsCB[si].cosAngle, g_spotLightsCB[si].cosFalloffAngle);
		
			Li = g_spotLightsCB[pi].color.rgb * lightIntensity;
			
			if (lightIntensity > 0.01f)
			{
				shadowRay.direction = normalize(g_spotLightsCB[si].position - shadowRayOrigin);
				shadowRayHit = TraceShadowRay(shadowRay, rayPayload.depth, distanceToLightSource);
		
				if (!shadowRayHit)
				{
					diffuse += Radiance(Li, shadowRay.direction, -WorldRayDirection(), normal, albedo, roughness, metallness, k);
				}
			}
		}
		
		rayPayload.color.rgb = diffuse;
		//rayPayload.color.rgb = attr.normal;
		rayPayload.color.a = 1.f;
	}
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

		[branch]
		if (tEnter >= 0)
		{
			tEnter += 0.01;

			currentVoxelPos = WorldSpaceToVoxelSpace(GetPositionAlongRay(localRay, tEnter));
			cellExit = tEnter;

			octreeNode = GetOctreeNode(currentVoxelPos);
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
			attr.unlit = false;
			
			ReportHit(tEnter, 0, attr);

			return;
		}
		
		[loop]
		for(int maxIterations = 255; maxIterations > 0; maxIterations--)
		{
			if(cellExit > tExit)
			{
				break;				
			}

			cellEnter = cellExit;

			[branch]
			if (IsValidCell(currentVoxelPos))
			{
				#ifdef SHADER_DEBUG
					if(DoesRayHitOctreeBounds(GetPositionAlongRay(localRay, cellEnter), octreeNode.nodePos, octreeNode.size, LINE_THICKNESS))
					{
						VolumeRaytracer::VPrimitiveAttributes attr;
						attr.normal = float3(1, 0, 0);
						attr.unlit = true;
					
						ReportHit(cellEnter, 0, attr);

						return;
					}
				#endif
				
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
					//attr.unlit = true;
					
					//ReportHit(10, 0, attr);

					//return;

					if (GetSurfaceIntersectionT(localRay, currentVoxelPos, octreeNode.size, cellEnter, cellExit, tHit))
					{
						VolumeRaytracer::VPrimitiveAttributes attr;
						attr.normal = GetNormal(currentVoxelPos, WorldSpaceToBottomLevelCellSpace(currentVoxelPos, octreeNode.size, GetPositionAlongRay(localRay, tHit)));
						attr.unlit = false;
						
						ReportHit(tHit, 0, attr);
						
						return;
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
				//attr.unlit = true;
				
				//ReportHit(10, 0, attr);
				
				return;
			}

			#ifdef SHADER_DEBUG
				if(!IsValidCell(nextVoxelPos) && DoesRayHitOctreeBounds(GetPositionAlongRay(localRay, cellExit), octreeNode.nodePos, octreeNode.size, LINE_THICKNESS))
				{
					VolumeRaytracer::VPrimitiveAttributes attr;
					attr.normal = float3(1, 0, 0);
					attr.unlit = true;
					
					ReportHit(cellExit, 0, attr);

					return;
				}
			#endif
			
			currentVoxelPos = nextVoxelPos;
			octreeNode = GetOctreeNode(nextVoxelPos);
		}
		
		if (maxIterations <= 0)
		{
			VolumeRaytracer::VPrimitiveAttributes attr;
			attr.normal = float3(1, 0, 0);
			attr.unlit = true;
			
			ReportHit(10, 0, attr);

			return;
		}
	}
}

[shader("intersection")]
void VRIntersectionShadowRay()
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

		[branch]
		if (tEnter >= 0)
		{
			tEnter += 0.01;

			currentVoxelPos = WorldSpaceToVoxelSpace(GetPositionAlongRay(localRay, tEnter));
			cellExit = tEnter;

			octreeNode = GetOctreeNode(currentVoxelPos);
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

		if (IsValidCell(currentVoxelPos) && IsSolidCell(currentVoxelPos, instance))
		{
			float3 rayPos = GetPositionAlongRay(localRay, tEnter - 0.1);

			VolumeRaytracer::VPrimitiveAttributes attr = { float3(0,0,0), true };
			ReportHit(tEnter, 0, attr);

			return;
		}

		[loop]
		for(int maxIterations = 255; maxIterations > 0; maxIterations--)
		{
			if(cellExit > tExit)
			{
				break;				
			}

			cellEnter = cellExit;

			[branch]
			if (IsValidCell(currentVoxelPos))
			{
				nextVoxelPos = GoToNextVoxel(localRay, octreeNode.nodePos, octreeNode.size, cellExit);

				if (HasIsoSurfaceInsideCell(instance, currentVoxelPos))
				{
					if (GetSurfaceIntersectionT(localRay, currentVoxelPos, octreeNode.size, cellEnter, cellExit, tHit))
					{
						VolumeRaytracer::VPrimitiveAttributes attr = { float3(0,0,0), true };
						ReportHit(tHit, 0, attr);
						
						return;
					}
				}
			}
			else
			{
				return;
			}

			currentVoxelPos = nextVoxelPos;
			octreeNode = GetOctreeNode(nextVoxelPos);
		}
		
		if (maxIterations <= 0)
		{
			VolumeRaytracer::VPrimitiveAttributes attr = { float3(0,0,0), true };
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