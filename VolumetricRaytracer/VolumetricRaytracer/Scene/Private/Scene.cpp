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
#include "VoxelObject.h"
#include "VoxelVolume.h"
#include "PointLight.h"
#include "SpotLight.h"

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

void VolumeRaytracer::Scene::VScene::UpdateMaterialOfVolume(std::weak_ptr<Voxel::VVoxelVolume> volume, const VMaterial& materialBefore, const VMaterial& newMaterial)
{
	std::shared_ptr<Voxel::VVoxelVolume> volumePtr = volume.lock();

	if (ReferencedVolumes.find(volumePtr) != ReferencedVolumes.end())
	{
		if (materialBefore.HasAlbedoTexture() && ReferencedTextures.find(materialBefore.AlbedoTexturePath) != ReferencedTextures.end())
		{
			ReferencedTextures[materialBefore.AlbedoTexturePath].Volumes.erase(volumePtr);

			if (ReferencedTextures[materialBefore.AlbedoTexturePath].Volumes.size() == 0)
			{
				ReferencedTextures.erase(materialBefore.AlbedoTexturePath);
			}
		}

		if (newMaterial.HasAlbedoTexture())
		{
			if (ReferencedTextures.find(newMaterial.AlbedoTexturePath) == ReferencedTextures.end())
			{
				ReferencedTextures[newMaterial.AlbedoTexturePath] = VTextureReference();
			}

			ReferencedTextures[newMaterial.AlbedoTexturePath].Volumes.insert(volumePtr);
		}


		if (materialBefore.HasNormalTexture() && ReferencedTextures.find(materialBefore.NormalTexturePath) != ReferencedTextures.end())
		{
			ReferencedTextures[materialBefore.NormalTexturePath].Volumes.erase(volumePtr);

			if (ReferencedTextures[materialBefore.NormalTexturePath].Volumes.size() == 0)
			{
				ReferencedTextures.erase(materialBefore.NormalTexturePath);
			}
		}

		if (newMaterial.HasNormalTexture())
		{
			if (ReferencedTextures.find(newMaterial.NormalTexturePath) == ReferencedTextures.end())
			{
				ReferencedTextures[newMaterial.NormalTexturePath] = VTextureReference();
			}

			ReferencedTextures[newMaterial.NormalTexturePath].Volumes.insert(volumePtr);
		}


		if (materialBefore.HasRMTexture() && ReferencedTextures.find(materialBefore.RMTexturePath) != ReferencedTextures.end())
		{
			ReferencedTextures[materialBefore.RMTexturePath].Volumes.erase(volumePtr);

			if (ReferencedTextures[materialBefore.RMTexturePath].Volumes.size() == 0)
			{
				ReferencedTextures.erase(materialBefore.RMTexturePath);
			}
		}

		if (newMaterial.HasRMTexture())
		{
			if (ReferencedTextures.find(newMaterial.RMTexturePath) == ReferencedTextures.end())
			{
				ReferencedTextures[newMaterial.RMTexturePath] = VTextureReference();
			}

			ReferencedTextures[newMaterial.RMTexturePath].Volumes.insert(volumePtr);
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

boost::unordered_set<std::wstring> VolumeRaytracer::Scene::VScene::GetAllReferencedGeometryTextures() const
{
	boost::unordered_set<std::wstring> texturePaths;

	for (auto& elem : ReferencedTextures)
	{
		texturePaths.insert(elem.first);
	}

	return texturePaths;
}

std::shared_ptr<VolumeRaytracer::VSerializationArchive> VolumeRaytracer::Scene::VScene::Serialize() const
{
	std::shared_ptr<VSerializationArchive> res = std::make_shared<VSerializationArchive>();
	res->BufferSize = 0;
	res->Buffer = nullptr;

	std::vector<VObjectPtr<Voxel::VVoxelVolume>> volumesList;

	struct ObjectVolumeRef
	{
		size_t VolumeIndex;
		std::shared_ptr<VVoxelObject> Object;
	};

	std::vector<ObjectVolumeRef> sceneObjects;
	
	for (auto& volume : ReferencedVolumes)
	{
		volumesList.push_back(volume.first);

		for (auto& obj : volume.second.Objects)
		{
			std::shared_ptr<VVoxelObject> voxObj = std::dynamic_pointer_cast<VVoxelObject>(obj);

			if (voxObj != nullptr)
			{
				ObjectVolumeRef volumeRef;
				volumeRef.VolumeIndex = volumesList.size() - 1;
				volumeRef.Object = voxObj;

				sceneObjects.push_back(volumeRef);
			}
		}
	}

	std::vector<std::shared_ptr<VLight>> directionalLights;
	std::vector<std::shared_ptr<VPointLight>> pointLights;
	std::vector<std::shared_ptr<VSpotLight>> spotLights;

	for (auto& object : PlacedObjects)
	{
		std::shared_ptr<VLight> dirLight;
		std::shared_ptr<VPointLight> pointLight;
		std::shared_ptr<VSpotLight> spotLight;

		pointLight = std::dynamic_pointer_cast<VPointLight>(object);
		spotLight = std::dynamic_pointer_cast<VSpotLight>(object);
		dirLight = std::dynamic_pointer_cast<VLight>(object);

		if (pointLight != nullptr)
		{
			pointLights.push_back(pointLight);
		}
		else if (spotLight != nullptr)
		{
			spotLights.push_back(spotLight);
		}
		else if (dirLight != nullptr)
		{
			directionalLights.push_back(dirLight);
		}
	}

	size_t num = volumesList.size();
	std::shared_ptr<VSerializationArchive> volumeCount = VSerializationArchive::From(&num);

	num = sceneObjects.size();
	std::shared_ptr<VSerializationArchive> sceneObjectsCount = VSerializationArchive::From(&num);

	num = pointLights.size();
	std::shared_ptr<VSerializationArchive> pointLightCount = VSerializationArchive::From(&num);

	num = spotLights.size();
	std::shared_ptr<VSerializationArchive> spotLightCount = VSerializationArchive::From(&num);

	num = directionalLights.size();
	std::shared_ptr<VSerializationArchive> dirLightCount = VSerializationArchive::From(&num);

	res->Properties["VCount"] = volumeCount;

	for (size_t i = 0; i < volumesList.size(); i++)
	{
		std::shared_ptr<VSerializationArchive> volumeArchive = std::static_pointer_cast<IVSerializable>(volumesList[i])->Serialize();

		std::stringstream ss;
		ss << "V_" << i;

		res->Properties[ss.str()] = volumeArchive;
	}

	res->Properties["OCount"] = sceneObjectsCount;

	for (size_t i = 0; i < sceneObjects.size(); i++)
	{
		std::shared_ptr<VSerializationArchive> volumeIndex = VSerializationArchive::From(&sceneObjects[i].VolumeIndex);

		std::stringstream ss;
		ss << "OI_" << i;

		res->Properties[ss.str()] = volumeIndex;

		std::shared_ptr<VSerializationArchive> objectArchive = std::static_pointer_cast<IVSerializable>(sceneObjects[i].Object)->Serialize();

		ss.str(std::string());
		ss.clear();
		ss << "O_" << i;

		res->Properties[ss.str()] = objectArchive;
	}

	res->Properties["LDCount"] = dirLightCount;

	for (size_t i = 0; i < directionalLights.size(); i++)
	{
		std::shared_ptr<VSerializationArchive> lightArchive = std::static_pointer_cast<IVSerializable>(directionalLights[i])->Serialize();

		std::stringstream ss;
		ss << "LD_" << i;

		res->Properties[ss.str()] = lightArchive;
	}

	res->Properties["LPCount"] = pointLightCount;

	for (size_t i = 0; i < pointLights.size(); i++)
	{
		std::shared_ptr<VSerializationArchive> lightArchive = std::static_pointer_cast<IVSerializable>(pointLights[i])->Serialize();

		std::stringstream ss;
		ss << "LP_" << i;

		res->Properties[ss.str()] = lightArchive;
	}

	res->Properties["LSCount"] = spotLightCount;

	for (size_t i = 0; i < spotLights.size(); i++)
	{
		std::shared_ptr<VSerializationArchive> lightArchive = std::static_pointer_cast<IVSerializable>(spotLights[i])->Serialize();

		std::stringstream ss;
		ss << "LS_" << i;

		res->Properties[ss.str()] = lightArchive;
	}

	return res;
}

void VolumeRaytracer::Scene::VScene::Deserialize(const std::wstring& sourcePath, std::shared_ptr<VSerializationArchive> archive)
{
	size_t volumesCount = archive->Properties["VCount"]->To<size_t>();
	size_t objectsCount = archive->Properties["OCount"]->To<size_t>();
	size_t dirLightCount = 0;
	size_t spotLightCount = 0;
	size_t pointLightCount = 0;

	if (archive->Properties.find("LDCount") != archive->Properties.end())
	{
		dirLightCount = archive->Properties["LDCount"]->To<size_t>();
	}

	if (archive->Properties.find("LPCount") != archive->Properties.end())
	{
		pointLightCount = archive->Properties["LPCount"]->To<size_t>();
	}

	if (archive->Properties.find("LSCount") != archive->Properties.end())
	{
		spotLightCount = archive->Properties["LSCount"]->To<size_t>();
	}

	std::vector<VObjectPtr<Voxel::VVoxelVolume>> volumes;

	for (size_t i = 0; i < volumesCount; i++)
	{
		std::stringstream ss;
		ss << "V_" << i;

		VObjectPtr<Voxel::VVoxelVolume> volume = VObject::CreateObject<Voxel::VVoxelVolume>(1, 1);
		std::static_pointer_cast<IVSerializable>(volume)->Deserialize(sourcePath, archive->Properties[ss.str()]);

		volumes.push_back(volume);
	}

	for (size_t i = 0; i < objectsCount; i++)
	{
		std::stringstream ss;
		ss << "OI_" << i;

		size_t volumeIndex = archive->Properties[ss.str()]->To<size_t>();

		ss.str(std::string());
		ss.clear();
		ss << "O_" << i;

		VObjectPtr<VVoxelObject> obj = SpawnObject<VVoxelObject>(VVector::ZERO, VQuat::IDENTITY, VVector::ONE);
		std::static_pointer_cast<IVSerializable>(obj)->Deserialize(sourcePath, archive->Properties[ss.str()]);

		obj->SetVoxelVolume(volumes[volumeIndex]);
	}

	for (size_t i = 0; i < dirLightCount; i++)
	{
		std::stringstream ss;
		ss << "LD_" << i;

		VObjectPtr<VLight> obj = SpawnObject<VLight>(VVector::ZERO, VQuat::IDENTITY, VVector::ONE);
		std::static_pointer_cast<IVSerializable>(obj)->Deserialize(sourcePath, archive->Properties[ss.str()]);

		SetActiveDirectionalLight(obj);
	}

	for (size_t i = 0; i < pointLightCount; i++)
	{
		std::stringstream ss;
		ss << "LP_" << i;

		VObjectPtr<VPointLight> obj = SpawnObject<VPointLight>(VVector::ZERO, VQuat::IDENTITY, VVector::ONE);
		std::static_pointer_cast<IVSerializable>(obj)->Deserialize(sourcePath, archive->Properties[ss.str()]);
	}

	for (size_t i = 0; i < spotLightCount; i++)
	{
		std::stringstream ss;
		ss << "LS_" << i;

		VObjectPtr<VSpotLight> obj = SpawnObject<VSpotLight>(VVector::ZERO, VQuat::IDENTITY, VVector::ONE);
		std::static_pointer_cast<IVSerializable>(obj)->Deserialize(sourcePath, archive->Properties[ss.str()]);
	}
}

VolumeRaytracer::VAABB VolumeRaytracer::Scene::VScene::GetSceneBounds() const
{
	VAABB res;

	for (auto& elem : PlacedObjects)
	{
		res = VAABB::Combine(res, elem->GetBounds());
	}

	return res;
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

		UpdateMaterialOfVolume(volume, VMaterial(), volumePtr->GetMaterial());
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
				UpdateMaterialOfVolume(volume, volumePtr->GetMaterial(), VMaterial());

				FrameRemovedVolumes.insert(volumePtr.get());
				FrameAddedVolumes.erase(volumePtr.get());
			}
		}
	}
}
