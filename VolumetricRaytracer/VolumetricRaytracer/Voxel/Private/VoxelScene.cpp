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
#include "Camera.h"

VolumeRaytracer::Voxel::VVoxelScene::VVoxelScene(const unsigned int& size, const float& volumeExtends) :Size(size),
	VolumeExtends(volumeExtends)
{}

unsigned int VolumeRaytracer::Voxel::VVoxelScene::GetSize() const
{
	return Size;
}

unsigned int VolumeRaytracer::Voxel::VVoxelScene::GetVoxelCount() const
{
	return Size * Size * Size;
}

float VolumeRaytracer::Voxel::VVoxelScene::GetVolumeExtends() const
{
	return VolumeExtends;
}

VolumeRaytracer::VAABB VolumeRaytracer::Voxel::VVoxelScene::GetSceneBounds() const
{
	VAABB res;

	res.SetCenterPosition(VVector::ZERO);
	res.SetExtends(VVector::ONE * VolumeExtends);

	return res;
}

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Scene::VCamera> VolumeRaytracer::Voxel::VVoxelScene::GetSceneCamera() const
{
	return Camera;
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

void VolumeRaytracer::Voxel::VVoxelScene::SetPathToEnvironmentMap(const std::wstring& path)
{
	EnvironmentMapPath = path;
}

std::wstring VolumeRaytracer::Voxel::VVoxelScene::GetEnvironmentMapPath() const
{
	return EnvironmentMapPath;
}

VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator VolumeRaytracer::Voxel::VVoxelScene::begin()
{
	return VVoxelSceneIterator(*this, 0);
}

VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator VolumeRaytracer::Voxel::VVoxelScene::end()
{
	return VVoxelSceneIterator(*this, GetVoxelCount());
}

void VolumeRaytracer::Voxel::VVoxelScene::Initialize()
{
	Camera = VObject::CreateObject<Scene::VCamera>();

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

VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator::VVoxelSceneIterator(VVoxelScene& scene, size_t index /*= 0*/)
	:Scene(scene),
	Index(index)
{

}

VolumeRaytracer::Voxel::VVoxelIteratorElement VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator::operator*() const
{
	if (Index < 0 || Index >= Scene.GetVoxelCount())
	{
		return VVoxelIteratorElement();
	}

	VVoxelIteratorElement elem;

	elem.Voxel = Scene.VoxelArr[Index];
	elem.Index = Index;
	
	VMathHelpers::Index1DTo3D(Index, Scene.GetSize(), Scene.GetSize(), elem.X, elem.Y, elem.Z);

	return elem;
}

VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator& VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator::operator++()
{
	++Index;
	return *this;
}

bool VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator::operator!=(const VVoxelSceneIterator& other) const
{
	return Index != other.Index;
}

VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator VolumeRaytracer::Voxel::VVoxelScene::VVoxelSceneIterator::operator++(int)
{
	VVoxelSceneIterator res(*this);
	operator++();

	return res;
}
