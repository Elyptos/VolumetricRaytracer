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

#include "DXConstants.h"

const std::wstring VolumeRaytracer::Renderer::DX::VDXConstants::HIT_GROUP(L"HitGroup_Ray");

const std::wstring VolumeRaytracer::Renderer::DX::VDXConstants::SHADOW_HIT_GROUP(L"HitGroup_ShadowRay");

const DXGI_FORMAT VolumeRaytracer::Renderer::DX::VDXConstants::BACK_BUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;

const unsigned int VolumeRaytracer::Renderer::DX::VDXConstants::BACK_BUFFER_COUNT = 3;

const std::wstring VolumeRaytracer::Renderer::DX::VDXConstants::SHADER_NAME_RAYGEN(L"VRRaygen");

const std::wstring VolumeRaytracer::Renderer::DX::VDXConstants::SHADER_NAME_CLOSEST_HIT(L"VRClosestHit");

const std::wstring VolumeRaytracer::Renderer::DX::VDXConstants::SHADER_NAME_INTERSECTION(L"VRIntersection");

const std::wstring VolumeRaytracer::Renderer::DX::VDXConstants::SHADER_NAME_MISS(L"VRMiss");

const std::wstring VolumeRaytracer::Renderer::DX::VDXConstants::SHADER_NAME_MISS_SHADOW(L"VRMissShadowRay");
