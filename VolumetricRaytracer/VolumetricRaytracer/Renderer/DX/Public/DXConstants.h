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
#include <string>
#include <dxgiformat.h>

namespace VolumeRaytracer
{
	namespace Renderer
	{
		namespace DX
		{
			class VDXConstants
			{
			public:
				static const std::wstring HIT_GROUP;
				static const std::wstring SHADOW_HIT_GROUP;

				static const DXGI_FORMAT BACK_BUFFER_FORMAT;
				static const unsigned int BACK_BUFFER_COUNT;

				static const size_t SRV_CV_UAV_HEAP_SIZE_PER_FRAME;
				static const size_t SAMPLER_HEAP_SIZE_PER_FRAME;

				static const std::wstring SHADER_NAME_RAYGEN;
				static const std::wstring SHADER_NAME_CLOSEST_HIT;
				static const std::wstring SHADER_NAME_INTERSECTION;
				static const std::wstring SHADER_NAME_INTERSECTION_SHADOW;
				static const std::wstring SHADER_NAME_MISS;
				static const std::wstring SHADER_NAME_MISS_SHADOW;
			};
		}
	}
}