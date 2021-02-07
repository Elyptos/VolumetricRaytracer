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

#include "Constants.hlsli"

float D(in float3 surfaceN, in float3 h, in float roughness)
{
	float a2 = roughness * roughness;
	float Ndoth = max(dot(surfaceN, h), 0.0f);
	
	float c = Ndoth * Ndoth * (a2 - 1.f) + 1.f;
	
	return a2 / max(PI * c * c, 0.001f);
}

float GSchlick(float nDotWo, in float k)
{
	return nDotWo / (nDotWo * (1.f - k) + k);
}

float GSmith(in float3 surfaceN, in float3 wo, in float3 wi, in float k)
{
	float dWo = max(dot(surfaceN, wo), 0.0f);
	float dWi = max(dot(surfaceN, wi), 0.0f);
	
	return GSchlick(dWo, k) * GSchlick(dWi, k);
}

float3 F(float3 f0, in float3 w0, in float3 h)
{
	float Wdoth = max(dot(w0, h), 0.0f);
	
	return f0 + (-f0 + 1.f) * pow(max(-Wdoth + 1.f, 0.0f), 5);
}

float3 BRDF(in float3 wi, in float3 wo, in float3 surfaceN, in float3 albedo, in float roughness, in float metallic, in float k)
{
	float3 h = normalize(wi + wo);
	float3 f0 = lerp(F0_DIEL, albedo, metallic);
	
	float3 d = D(surfaceN, h, roughness);
	float3 f = F(f0, wo, h);
	float3 g = GSmith(surfaceN, wo, wi, k);
	
	float3 lambert = albedo / PI;
	float3 cook = (d * f * g) / max((4 * max(dot(wo, surfaceN), 0.0f) * max(dot(wi, surfaceN), 0.0f)), 0.0001f);
	
	float3 kd = float3(1.0f, 1.0f, 1.0f) - f;
	kd *= (1.0f - metallic);
	
	return lambert * kd + cook * f;
}

float3 Radiance(in float3 Li, in float3 wi, in float3 wo, in float3 surfaceN, in float3 albedo, in float roughness, in float metallic, in float k)
{
	return BRDF(wi, wo, surfaceN, albedo, roughness, metallic, k) * Li /** dot(surfaceN, wi)*/;
}