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

				static unsigned int MAX_RAY_RECURSION_DEPTH;

				static DXGI_FORMAT BACK_BUFFER_FORMAT;
			};
		}
	}
}