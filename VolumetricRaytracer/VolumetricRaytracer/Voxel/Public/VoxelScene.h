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

#include "Voxel.h"
#include "Object.h"
#include "AABB.h"

namespace VolumeRaytracer
{
	namespace Voxel
	{
		class VVoxelScene : VObject 
		{
		public:
			VVoxelScene(const unsigned int& size, const float& voxelSize);

			unsigned int GetSize() const;
			unsigned int GetVoxelCount() const;

			VAABB GetSceneBounds() const;

			void SetVoxel(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos, const VVoxel& voxel);
			VVoxel GetVoxel(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos) const;
			bool IsValidVoxelIndex(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos) const;

		protected:
			void Initialize() override;
			void BeginDestroy() override;

		private:
			unsigned int Size = 0;
			float VoxelSize = 0;
			VVoxel* VoxelArr;
		};
	}
}