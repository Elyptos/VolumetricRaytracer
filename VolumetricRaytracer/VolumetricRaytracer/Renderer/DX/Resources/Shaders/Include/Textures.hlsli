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


float3 TriSampleTexture(in uint textureID, in float2 scale, in float3 worldPos, in float3 normal)
{
	float2 uvX = worldPos.zy / scale;
	float2 uvY = worldPos.xz / scale;
	float2 uvZ = worldPos.xy / scale;
	
	float3 tX = g_geometryTextures[textureID].SampleLevel(g_geometrySampler, uvX, 0.f).rgb;
	float3 tY = g_geometryTextures[textureID].SampleLevel(g_geometrySampler, uvY, 0.f).rgb;
	float3 tZ = g_geometryTextures[textureID].SampleLevel(g_geometrySampler, uvZ, 0.f).rgb;
	
	float3 blend = abs(normal);
	blend = blend / (blend.x + blend.y + blend.z);
	
	return tX * blend.x + tY * blend.y + tZ * blend.z;
}

float3 TriSampleNormal(in uint textureID, in float2 scale, in float3 worldPos, in float3 normal)
{
	float2 uvX = worldPos.zy / scale;
	float2 uvY = worldPos.xz / scale;
	float2 uvZ = worldPos.xy / scale;
	
	float3 tX = g_geometryTextures[textureID].SampleLevel(g_geometrySampler, uvX, 0.f).rgb * 2.0f - 1.0f;
	float3 tY = g_geometryTextures[textureID].SampleLevel(g_geometrySampler, uvY, 0.f).rgb * 2.0f - 1.0f;
	float3 tZ = g_geometryTextures[textureID].SampleLevel(g_geometrySampler, uvZ, 0.f).rgb * 2.0f - 1.0f;
		
	float3 blend = abs(normal);
	blend = blend / (blend.x + blend.y + blend.z);
	
	float3 tNormal = normalize(tX * blend.x + tY * blend.y + tZ * blend.z);
	tNormal = tNormal.zxy;
	
	float4 quat = fromX(normal);
	
	return rotate_vector(tNormal, quat);
}