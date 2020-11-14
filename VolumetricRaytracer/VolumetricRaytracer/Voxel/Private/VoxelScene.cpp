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

#include "VoxelScene.h"
#include "MathHelpers.h"

VolumeRaytracer::Voxel::VVoxelScene::VVoxelScene(const unsigned int& size, const float& voxelSize)
	:Size(size),
	VoxelSize(voxelSize)
{}

unsigned int VolumeRaytracer::Voxel::VVoxelScene::GetSize() const
{
	return Size;
}

unsigned int VolumeRaytracer::Voxel::VVoxelScene::GetVoxelCount() const
{
	return Size * Size * Size;
}

VolumeRaytracer::VAABB VolumeRaytracer::Voxel::VVoxelScene::GetSceneBounds() const
{
	VAABB res;

	res.SetCenterPosition(VVector::ZERO);
	res.SetExtends(VVector::ONE * GetSize() * VoxelSize * 0.5f);

	return res;
}

void VolumeRaytracer::Voxel::VVoxelScene::SetVoxel(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos, const VVoxel& voxel)
{
	if (IsValidVoxelIndex(xPos, yPos, zPos))
	{
		unsigned int index = VMathHelpers::Index3DTo1D(xPos, yPos, zPos, GetSize(), GetSize());

		VoxelArr[index] = voxel;
	}
}

VolumeRaytracer::Voxel::VVoxel VolumeRaytracer::Voxel::VVoxelScene::GetVoxel(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos) const
{
	if (IsValidVoxelIndex(xPos, yPos, zPos))
	{
		unsigned int index = VMathHelpers::Index3DTo1D(xPos, yPos, zPos, GetSize(), GetSize());

		return VoxelArr[index];
	}

	return VVoxel();
}

bool VolumeRaytracer::Voxel::VVoxelScene::IsValidVoxelIndex(const unsigned int& xPos, const unsigned int& yPos, const unsigned int& zPos) const
{
	return xPos < Size && yPos < Size && zPos < Size;
}

void VolumeRaytracer::Voxel::VVoxelScene::Initialize()
{
	if (GetSize() > 0)
	{
		VoxelArr = new VVoxel[GetVoxelCount()];
	}
	else
	{
		VoxelArr = nullptr;
	}
}

void VolumeRaytracer::Voxel::VVoxelScene::BeginDestroy()
{
	if (VoxelArr != nullptr)
	{
		delete[] VoxelArr;
		VoxelArr = nullptr;
	}
}
