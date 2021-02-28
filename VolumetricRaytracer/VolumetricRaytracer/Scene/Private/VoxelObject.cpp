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
#include "VoxelVolume.h"

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

std::shared_ptr<VolumeRaytracer::VSerializationArchive> VolumeRaytracer::Scene::VVoxelObject::Serialize() const
{
	std::shared_ptr<VSerializationArchive> res = std::make_shared<VSerializationArchive>();
	res->BufferSize = 0;
	res->Buffer = nullptr;

	std::shared_ptr<VSerializationArchive> pos = std::make_shared<VSerializationArchive>();
	std::shared_ptr<VSerializationArchive> scale = std::make_shared<VSerializationArchive>();
	std::shared_ptr<VSerializationArchive> rot = std::make_shared<VSerializationArchive>();

	pos->BufferSize = sizeof(VVector);
	scale->BufferSize = sizeof(VVector);
	rot->BufferSize = sizeof(VQuat);

	pos->Buffer = new char[sizeof(VVector)];
	scale->Buffer = new char[sizeof(VVector)];
	rot->Buffer = new char[sizeof(VQuat)];

	memcpy(pos->Buffer, &Position, sizeof(VVector));
	memcpy(scale->Buffer, &Scale, sizeof(VVector));
	memcpy(rot->Buffer, &Rotation, sizeof(VQuat));

	res->Properties["Position"] = pos;
	res->Properties["Scale"] = scale;
	res->Properties["Rotation"] = rot;

	return res;
}

void VolumeRaytracer::Scene::VVoxelObject::Deserialize(std::shared_ptr<VSerializationArchive> archive)
{
	memcpy(&Position, archive->Properties["Position"]->Buffer, sizeof(VVector));
	memcpy(&Scale, archive->Properties["Scale"]->Buffer, sizeof(VVector));
	memcpy(&Rotation, archive->Properties["Rotation"]->Buffer, sizeof(VQuat));
}

VolumeRaytracer::VAABB VolumeRaytracer::Scene::VVoxelObject::GetBounds() const
{
	if (VoxelVolume != nullptr)
	{
		VAABB volumeBounds = VoxelVolume->GetVolumeBounds();

		return VAABB::Transform(volumeBounds, Position, Scale, Rotation);
	}
	else
	{
		return VLevelObject::GetBounds();
	}
}

void VolumeRaytracer::Scene::VVoxelObject::Initialize()
{
	
}

void VolumeRaytracer::Scene::VVoxelObject::BeginDestroy()
{
	
}

