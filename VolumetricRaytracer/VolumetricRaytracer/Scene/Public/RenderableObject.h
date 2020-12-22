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
#include "Object.h"

namespace VolumeRaytracer
{
	namespace Voxel
	{
		class VVoxelVolume;
	}

	namespace Scene
	{
		class IVRenderableObject
		{
		public:
			virtual std::weak_ptr<Voxel::VVoxelVolume> GetVoxelVolume() = 0;
		};
	}
}