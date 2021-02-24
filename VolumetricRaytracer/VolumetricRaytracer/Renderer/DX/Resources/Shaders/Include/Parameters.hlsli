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

RaytracingAccelerationStructure g_scene : register(t0, space0);

Texture3D<uint4> g_voxelVolume[VolumeRaytracer::MaxAllowedObjectData] : register(t65, space0);
Texture3D<uint4> g_traversalVolume[VolumeRaytracer::MaxAllowedObjectData] : register(t85, space0);
ConstantBuffer<VolumeRaytracer::VPointLightBuffer> g_pointLightsCB[VolumeRaytracer::MaxAllowedPointLights] : register(b1);
ConstantBuffer<VolumeRaytracer::VSpotLightBuffer> g_spotLightsCB[VolumeRaytracer::MaxAllowedSpotLights] : register(b6);
ConstantBuffer<VolumeRaytracer::VGeometryConstantBuffer> g_geometryCB[VolumeRaytracer::MaxAllowedObjectData] : register(b11);

TextureCube g_envMap : register(t1, space0);
SamplerState g_envMapSampler : register(s0);

Texture2D<float4> g_geometryTextures[63] : register(t2);
SamplerState g_geometrySampler : register(s1);

RWTexture2D<float4> g_renderTarget : register(u0);
ConstantBuffer<VolumeRaytracer::VSceneConstantBuffer> g_sceneCB : register(b0);

