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
#include <stdint.h>
#include "MathHelpers.h"

namespace VolumeRaytracer
{
	namespace Voxel
	{
		struct VVoxel
		{
		public:
			static const float DEFAULT_DENSITY;

			uint8_t Material = 0;
			float Density = DEFAULT_DENSITY;
		};

		struct VCell
		{
		public:
			VVoxel Voxels[8];

			bool HasSurface() const;

			void FillWithVoxel(const VVoxel& voxel);

			static uint8_t GetVoxelIndex(const VIntVector& index3D);
			static const VIntVector VOXEL_COORDS[8];

			VVoxel GetAvgVoxel() const;
		};
	}
}