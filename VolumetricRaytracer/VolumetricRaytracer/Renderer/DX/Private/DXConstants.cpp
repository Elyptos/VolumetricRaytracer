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

unsigned int VolumeRaytracer::Renderer::DX::VDXConstants::MAX_RAY_RECURSION_DEPTH = 3;

DXGI_FORMAT VolumeRaytracer::Renderer::DX::VDXConstants::BACK_BUFFER_FORMAT = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
