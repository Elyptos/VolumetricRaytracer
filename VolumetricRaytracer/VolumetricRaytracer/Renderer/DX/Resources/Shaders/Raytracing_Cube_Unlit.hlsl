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
		float3 albedo = g_geometryCB[InstanceID()].tint.rgb * TriSampleTexture(g_geometryCB[InstanceID()].albedoTexture, g_geometryCB[InstanceID()].textureScale, hitPosition, attr.normal);
		
		rayPayload.color.rgb = albedo;
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
		float3 voxelNormal;
		float3 nextNormal;
		
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
		
		float3 rayPos = GetPositionAlongRay(localRay, tEnter - 0.1);

		voxelNormal = sign(rayPos - volumeAABB[1]);

		if (voxelNormal.x < 0)
		{
			voxelNormal.x = rayPos.x < volumeAABB[0].x ? -1 : 0;
		}

		if (voxelNormal.y < 0)
		{
			voxelNormal.y = rayPos.y < volumeAABB[0].y ? -1 : 0;
		}

		if (voxelNormal.z < 0)
		{
			voxelNormal.z = rayPos.z < volumeAABB[0].z ? -1 : 0;
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
			if (IsValidVoxelIndex(currentVoxelPos))
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
				
				nextVoxelPos = GoToNextVoxel(localRay, octreeNode.nodePos, octreeNode.size, cellExit, nextNormal);
				
				if (GetVoxelDensity(currentVoxelPos, instance) <= 0.f)
				{
					VolumeRaytracer::VPrimitiveAttributes attr;
					attr.normal = voxelNormal;
					attr.unlit = false;
						
					ReportHit(cellEnter, 0, attr);
						
					return;
				}
			}
			else
			{
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
			voxelNormal = nextNormal;
			
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
		
		float3 rayPos = GetPositionAlongRay(localRay, tEnter - 0.1);
		
		[loop]
		for(int maxIterations = 255; maxIterations > 0; maxIterations--)
		{
			if(cellExit > tExit)
			{
				break;				
			}

			cellEnter = cellExit;
			
			[branch]
			if (IsValidVoxelIndex(currentVoxelPos))
			{
				nextVoxelPos = GoToNextVoxel(localRay, octreeNode.nodePos, octreeNode.size, cellExit);
				
				if (GetVoxelDensity(currentVoxelPos, instance) <= 0.f)
				{
					VolumeRaytracer::VPrimitiveAttributes attr;
					attr.normal = float3(0,0,0);
					attr.unlit = true;
						
					ReportHit(cellEnter, 0, attr);
						
					return;
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
			VolumeRaytracer::VPrimitiveAttributes attr;
			attr.normal = float3(0, 0, 0);
			attr.unlit = true;
			
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