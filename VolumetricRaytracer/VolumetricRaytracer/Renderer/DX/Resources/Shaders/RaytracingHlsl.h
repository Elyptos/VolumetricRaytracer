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

#pragma once

#ifdef HLSL
typedef float2 XMFLOAT2;
typedef float3 XMFLOAT3;
typedef float4 XMFLOAT4;
typedef float4 XMVECTOR;
typedef int3 XMINT3;
typedef float4x4 XMMATRIX;
typedef uint UINT;
#else
	#include <DirectXMath.h>
	using namespace DirectX;

	typedef unsigned int UINT;
#endif

#define MAX_RAY_RECURSION_DEPTH 3

namespace VolumeRaytracer
{
	struct VPrimitiveAttributes
	{
		XMFLOAT3 normal;
		bool unlit;
	};

	struct VRayPayload
	{
		XMFLOAT4 color;
		UINT depth;
	};

	struct VShadowRayPayload
	{
		bool hit;
	};

	struct VSceneConstantBuffer
	{
		XMMATRIX viewMatrixInverted;
		XMMATRIX projectionMatrixInverted;
		XMVECTOR cameraPosition;
		XMFLOAT3 dirLightDirection;
		float dirLightStrength;
		float numPointLights;
		float numSpotLights;
	};

	struct VPointLightBuffer
	{
		XMFLOAT4 color;
		float lightIntensity;
		float attLinear;
		float attExp;
		float padding1;
		XMFLOAT3 position;
	};

	struct VSpotLightBuffer
	{
		XMFLOAT4 color;
		float lightIntensity;
		float attLinear;
		float attExp;
		float cosAngle;
		float cosFalloffAngle;
		XMFLOAT3 position;
		XMFLOAT3 forward;
	};

	struct VGeometryConstantBuffer
	{
		XMFLOAT4 tint;
		float roughness;
		float metallness;
		float k;
		UINT voxelAxisCount;
		float volumeExtend;
		float distanceBtwVoxels;
		UINT octreeDepth;
	};

	static const XMFLOAT4 BackgroundColor = XMFLOAT4(0.8f, 0.9f, 1.0f, 1.0f);
	static const XMFLOAT3 CubeNormals[] = {
		XMFLOAT3(1.0f, 0.0f, 0.0f),
		XMFLOAT3(-1.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, -1.0f, 0.0f),
		XMFLOAT3(0.0f, 0.0f, 1.0f),
		XMFLOAT3(0.0f, 0.0f, -1.0f)
	};

	static const UINT MaxAllowedObjectData = 20;
	static const UINT MaxAllowedSpotLights = 5;
	static const UINT MaxAllowedPointLights = 5;
}