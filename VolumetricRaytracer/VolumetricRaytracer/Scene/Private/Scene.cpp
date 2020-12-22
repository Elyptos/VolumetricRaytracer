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

#include "Scene.h"
#include "Camera.h"
#include "Light.h"
#include "Logger.h"
#include "RenderableObject.h"
#include <algorithm>

void VolumeRaytracer::Scene::VScene::Tick(const float& deltaSeconds)
{
	
}

const bool VolumeRaytracer::Scene::VScene::CanEverTick() const
{
	return true;
}

bool VolumeRaytracer::Scene::VScene::ShouldTick() const
{
	return true;
}

void VolumeRaytracer::Scene::VScene::DestroyObject(std::weak_ptr<VLevelObject> obj)
{
	if (!obj.expired())
	{
		VObjectPtr<VLevelObject> objPtr = obj.lock();

		if (PlacedObjects.find(objPtr) != PlacedObjects.end())
		{
			std::shared_ptr<IVRenderableObject> renderableObject = std::dynamic_pointer_cast<IVRenderableObject>(objPtr);

			if (renderableObject != nullptr && !renderableObject->GetVoxelVolume().expired())
			{
				RemoveVolumeReference_Internal(renderableObject, renderableObject->GetVoxelVolume());
			}

			objPtr->RemoveFromScene();
			PlacedObjects.erase(objPtr);

			if (FrameAddedObjects.find(objPtr.get()) != FrameAddedObjects.end())
			{
				FrameAddedObjects.erase(objPtr.get());
			}

			if (FrameDirtyObjects.find(objPtr.get()) != FrameDirtyObjects.end())
			{
				FrameDirtyObjects.erase(objPtr.get());
			}

			FrameRemovedObjects.insert(objPtr.get());
		}
	}
}

void VolumeRaytracer::Scene::VScene::SetEnvironmentTexture(VObjectPtr<VTextureCube> texture)
{
	EnvironmentTexture = texture;
}

VolumeRaytracer::VObjectPtr<VolumeRaytracer::VTextureCube> VolumeRaytracer::Scene::VScene::GetEnvironmentTexture() const
{
	return EnvironmentTexture;
}

void VolumeRaytracer::Scene::VScene::SetActiveSceneCamera(std::weak_ptr<VCamera> camera)
{
	if (!camera.expired())
	{
		std::shared_ptr<VCamera> cameraPtr = camera.lock();

		if (cameraPtr->GetScene().lock().get() == this)
		{
			ActiveCamera = camera;
		}
		else
		{
			V_LOG_ERROR("Can´t set camera as active camera because it does not belong to this scene!");
		}
	}
}

void VolumeRaytracer::Scene::VScene::SetActiveDirectionalLight(std::weak_ptr<VLight> light)
{
	if (!light.expired())
	{
		std::shared_ptr<VLight> lightPtr = light.lock();

		if (lightPtr->GetScene().lock().get() == this)
		{
			ActiveDirectionalLight = light;
		}
		else
		{
			V_LOG_ERROR("Can´t set light as active directional light because it does not belong to this scene!");
		}
	}
}

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Scene::VCamera> VolumeRaytracer::Scene::VScene::GetActiveCamera() const
{
	return ActiveCamera.lock();
}

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Scene::VLight> VolumeRaytracer::Scene::VScene::GetActiveDirectionalLight() const
{
	return ActiveDirectionalLight.lock();
}

void VolumeRaytracer::Scene::VScene::PostRender()
{
	ClearFrameCaches();
}

bool VolumeRaytracer::Scene::VScene::ContainsObject(std::weak_ptr<VLevelObject> object) const
{
	return !object.expired() && PlacedObjects.find(object.lock()) != PlacedObjects.end();
}

void VolumeRaytracer::Scene::VScene::UpdateVoxelVolumeReference(std::weak_ptr<Voxel::VVoxelVolume> prevVolume, std::weak_ptr<IVRenderableObject> renderableObject)
{
	if (!renderableObject.expired())
	{
		std::shared_ptr<VLevelObject> levelObject = std::dynamic_pointer_cast<VLevelObject>(renderableObject.lock());

		if (levelObject != nullptr)
		{
			if (ContainsObject(levelObject))
			{
				if (!prevVolume.expired())
				{
					RemoveVolumeReference_Internal(renderableObject, prevVolume);
				}

				std::weak_ptr<Voxel::VVoxelVolume> volume = renderableObject.lock()->GetVoxelVolume();

				if (!volume.expired())
				{
					AddVolumeReference_Internal(renderableObject, volume);
				}
			}
		}
	}
}

void VolumeRaytracer::Scene::VScene::RemoveVoxelVolumeReference(std::weak_ptr<Voxel::VVoxelVolume> volume, std::weak_ptr<IVRenderableObject> renderableObject)
{
	if (!renderableObject.expired())
	{
		std::shared_ptr<VLevelObject> levelObject = std::dynamic_pointer_cast<VLevelObject>(renderableObject.lock());

		if (levelObject != nullptr)
		{
			if (ContainsObject(levelObject))
			{
				if (!volume.expired())
				{
					RemoveVolumeReference_Internal(renderableObject, volume);
				}
			}
		}
	}
}

std::vector<std::weak_ptr<VolumeRaytracer::Scene::VLevelObject>> VolumeRaytracer::Scene::VScene::GetAllPlacedObjects() const
{
	std::vector<std::weak_ptr<VLevelObject>> res;

	for (auto& elem : PlacedObjects)
	{
		res.push_back(elem);
	}

	return res;
}

boost::unordered_set<VolumeRaytracer::Scene::VLevelObject*> VolumeRaytracer::Scene::VScene::GetObjectsAddedDuringFrame() const
{
	return FrameAddedObjects;
}

boost::unordered_set<VolumeRaytracer::Scene::VLevelObject*> VolumeRaytracer::Scene::VScene::GetObjectsRemovedDuringFrame() const
{
	return FrameRemovedObjects;
}

boost::unordered_set<VolumeRaytracer::Scene::VLevelObject*> VolumeRaytracer::Scene::VScene::GetAllDirtyObjects() const
{
	return FrameDirtyObjects;
}

std::vector<std::weak_ptr<VolumeRaytracer::Voxel::VVoxelVolume>> VolumeRaytracer::Scene::VScene::GetAllRegisteredVolumes() const
{
	std::vector<std::weak_ptr<VolumeRaytracer::Voxel::VVoxelVolume>> res;

	for (auto& elem : ReferencedVolumes)
	{
		res.push_back(elem.first);
	}

	return res;
}

boost::unordered_set<VolumeRaytracer::Voxel::VVoxelVolume*> VolumeRaytracer::Scene::VScene::GetVolumesAddedDuringFrame() const
{
	return FrameAddedVolumes;
}

boost::unordered_set<VolumeRaytracer::Voxel::VVoxelVolume*> VolumeRaytracer::Scene::VScene::GetVolumesRemovedDuringFrame() const
{
	return FrameRemovedVolumes;
}

void VolumeRaytracer::Scene::VScene::Initialize()
{
	
}

void VolumeRaytracer::Scene::VScene::BeginDestroy()
{
	
}

void VolumeRaytracer::Scene::VScene::ClearFrameCaches()
{
	FrameAddedObjects.clear();
	FrameRemovedObjects.clear();
	FrameDirtyObjects.clear();

	FrameAddedVolumes.clear();
	FrameRemovedVolumes.clear();
}

void VolumeRaytracer::Scene::VScene::AddVolumeReference_Internal(std::weak_ptr<IVRenderableObject> renderableObject, std::weak_ptr<Voxel::VVoxelVolume> volume)
{
	std::shared_ptr<Voxel::VVoxelVolume> volumePtr = volume.lock();
	std::shared_ptr<IVRenderableObject> renderableObjectPtr = renderableObject.lock();

	if (ReferencedVolumes.find(volumePtr) != ReferencedVolumes.end())
	{
		boost::unordered_set<std::shared_ptr<IVRenderableObject>>& objects = ReferencedVolumes[volumePtr].Objects;

		if (objects.find(renderableObjectPtr) == objects.end())
		{
			objects.insert(renderableObjectPtr);
		}
	}
	else
	{
		VRenderableObjectContainer container;
		container.Objects.insert(renderableObjectPtr);

		ReferencedVolumes[volume.lock()] = container;

		FrameRemovedVolumes.erase(volumePtr.get());
		FrameAddedVolumes.insert(volumePtr.get());
	}
}

void VolumeRaytracer::Scene::VScene::RemoveVolumeReference_Internal(std::weak_ptr<IVRenderableObject> renderableObject, std::weak_ptr<Voxel::VVoxelVolume> volume)
{
	std::shared_ptr<Voxel::VVoxelVolume> volumePtr = volume.lock();
	std::shared_ptr<IVRenderableObject> renderableObjectPtr = renderableObject.lock();

	if (ReferencedVolumes.find(volumePtr) != ReferencedVolumes.end())
	{
		boost::unordered_set<std::shared_ptr<IVRenderableObject>>& objects = ReferencedVolumes[volumePtr].Objects;

		if (objects.find(renderableObjectPtr) != objects.end())
		{
			objects.erase(renderableObjectPtr);

			if (objects.size() == 0)
			{
				FrameRemovedVolumes.insert(volumePtr.get());
				FrameAddedVolumes.erase(volumePtr.get());
			}
		}
	}
}
