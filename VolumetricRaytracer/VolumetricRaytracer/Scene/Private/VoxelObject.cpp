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

#include "VoxelObject.h"
#include "Scene.h"

void VolumeRaytracer::Scene::VVoxelObject::SetVoxelVolume(VObjectPtr<Voxel::VVoxelVolume> volume)
{
	if (!GetScene().expired())
	{
		std::shared_ptr<IVRenderableObject> renderablePtr = std::static_pointer_cast<IVRenderableObject>(shared_from_this());

		std::weak_ptr<VolumeRaytracer::Voxel::VVoxelVolume> oldVolume = GetVoxelVolume();
		VoxelVolume = volume;

		GetScene().lock()->UpdateVoxelVolumeReference(oldVolume, renderablePtr);
	}
}

std::weak_ptr<VolumeRaytracer::Voxel::VVoxelVolume> VolumeRaytracer::Scene::VVoxelObject::GetVoxelVolume()
{
	return VoxelVolume;
}

void VolumeRaytracer::Scene::VVoxelObject::Initialize()
{
	
}

void VolumeRaytracer::Scene::VVoxelObject::BeginDestroy()
{
	
}

