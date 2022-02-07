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
#include "LevelObject.h"
#include "ISerializable.h"
#include <vector>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "Material.h"

namespace VolumeRaytracer
{
	class VTextureCube;

	namespace Voxel
	{
		class VVoxelVolume;
	}

	namespace Scene
	{
		class VCamera;
		class VLight;
		class IVRenderableObject;

		struct VRenderableObjectContainer
		{
		public:
			boost::unordered_set<std::shared_ptr<IVRenderableObject>> Objects;
		};

		struct VTextureReference
		{
		public:
			boost::unordered_set<std::shared_ptr<Voxel::VVoxelVolume>> Volumes;
		};

		class VScene : public VObject, public std::enable_shared_from_this<VScene>, public IVSerializable
		{

		public:
			void Tick(const float& deltaSeconds) override;


			const bool CanEverTick() const override;


			bool ShouldTick() const override;

			template<typename T, class... _Types>
			VObjectPtr<T> SpawnObject(const VVector& location, const VQuat& rotation, const VVector& scale, _Types&&... args)
			{
				static_assert(std::is_base_of<VLevelObject, T>::value, "T must inherit from VLevelObject");

				VObjectPtr<VLevelObject> obj = VObject::CreateObject<T>(std::forward<_Types>(args)...);

				obj->Position = location;
				obj->Rotation = rotation;
				obj->Scale = scale;

				PlacedObjects.insert(obj);

				obj->SetScene(shared_from_this());

				FrameAddedObjects.insert(obj.get());

				if (FrameRemovedObjects.find(obj.get()) != FrameRemovedObjects.end())
				{
					FrameRemovedObjects.erase(obj.get());
				}

				std::shared_ptr<IVRenderableObject> renderableObject = std::dynamic_pointer_cast<IVRenderableObject>(obj);

				if (renderableObject != nullptr && !renderableObject->GetVoxelVolume().expired())
				{
					UpdateVoxelVolumeReference(std::weak_ptr<Voxel::VVoxelVolume>(), renderableObject);
				}

				return std::static_pointer_cast<T>(obj);
			}

			void DestroyObject(std::weak_ptr<VLevelObject> obj);

			void SetEnvironmentTexture(VObjectPtr<VTextureCube> texture);
			VObjectPtr<VTextureCube> GetEnvironmentTexture() const;

			void SetActiveSceneCamera(std::weak_ptr<VCamera> camera);
			void SetActiveDirectionalLight(std::weak_ptr<VLight> light);

			VObjectPtr<VCamera> GetActiveCamera() const;
			VObjectPtr<VLight> GetActiveDirectionalLight() const;

			void PostRender() override;

			bool ContainsObject(std::weak_ptr<VLevelObject> object) const;

			void UpdateVoxelVolumeReference(std::weak_ptr<Voxel::VVoxelVolume> prevVolume, std::weak_ptr<IVRenderableObject> renderableObject);
			void RemoveVoxelVolumeReference(std::weak_ptr<Voxel::VVoxelVolume> volume, std::weak_ptr<IVRenderableObject> renderableObject);

			void UpdateMaterialOfVolume(std::weak_ptr<Voxel::VVoxelVolume> volume, const VMaterial& materialBefore, const VMaterial& newMaterial);

			std::vector<std::weak_ptr<VLevelObject>> GetAllPlacedObjects() const;
			boost::unordered_set<VLevelObject*> GetObjectsAddedDuringFrame() const;
			boost::unordered_set<VLevelObject*> GetObjectsRemovedDuringFrame() const;
			boost::unordered_set<VLevelObject*> GetAllDirtyObjects() const;

			std::vector<std::weak_ptr<Voxel::VVoxelVolume>> GetAllRegisteredVolumes() const;
			boost::unordered_set<Voxel::VVoxelVolume*> GetVolumesAddedDuringFrame() const;
			boost::unordered_set<Voxel::VVoxelVolume*> GetVolumesRemovedDuringFrame() const;

			boost::unordered_set<std::wstring> GetAllReferencedGeometryTextures() const;

			std::shared_ptr<VSerializationArchive> Serialize() const override;


			void Deserialize(const std::wstring& sourcePath, std::shared_ptr<VSerializationArchive> archive) override;

			VAABB GetSceneBounds() const;

		protected:
			void Initialize() override;

			void BeginDestroy() override;

		private:
			void ClearFrameCaches();

			void AddVolumeReference_Internal(std::weak_ptr<IVRenderableObject> renderableObject, std::weak_ptr<Voxel::VVoxelVolume> volume);
			void RemoveVolumeReference_Internal(std::weak_ptr<IVRenderableObject> renderableObject, std::weak_ptr<Voxel::VVoxelVolume> volume);

		private:
			boost::unordered_set<VObjectPtr<VLevelObject>> PlacedObjects;
			boost::unordered_set<VLevelObject*> FrameAddedObjects;
			boost::unordered_set<VLevelObject*> FrameRemovedObjects;
			boost::unordered_set<VLevelObject*> FrameDirtyObjects;

			boost::unordered_map<VObjectPtr<Voxel::VVoxelVolume>, VRenderableObjectContainer> ReferencedVolumes;
			boost::unordered_set<Voxel::VVoxelVolume*> FrameAddedVolumes;
			boost::unordered_set<Voxel::VVoxelVolume*> FrameRemovedVolumes;

			boost::unordered_map<std::wstring, VTextureReference> ReferencedTextures;

			std::weak_ptr<VCamera> ActiveCamera;
			std::weak_ptr<VLight> ActiveDirectionalLight;

			VObjectPtr<VTextureCube> EnvironmentTexture;
		};
	}
}