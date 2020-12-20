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

#include "RScene.h"
#include "DXHelper.h"
#include "Material.h"
#include <vector>
#include <DirectXMath.h>
#include <boost/lockfree/queue.hpp>

struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct ID3D12Device5;

namespace VolumeRaytracer
{
	namespace Scene
	{
		class IVRenderableObject;
	}

	namespace Renderer
	{
		namespace DX
		{
			class VDXRenderer;
			class VDXDescriptorHeap;
			class VDXTextureCube;
			class VDXTexture3DFloat;
			class VDXVoxelVolume;
			class VDXLevelObject;

			struct VRDXSceneObjectDescriptorHandles
			{
			public:
				D3D12_CPU_DESCRIPTOR_HANDLE VoxelVolumeHandleCPU;
				D3D12_GPU_DESCRIPTOR_HANDLE VoxelVolumeHandleGPU;
				D3D12_CPU_DESCRIPTOR_HANDLE GeometryHandleCPU;
				D3D12_GPU_DESCRIPTOR_HANDLE GeometryHandleGPU;
			};

			class VRDXSceneObjectResourcePool
			{
			public:
				VRDXSceneObjectResourcePool(CPtr<ID3D12Device5> dxDevice, const size_t& maxObjects);
				~VRDXSceneObjectResourcePool();

				VRDXSceneObjectDescriptorHandles GetObjectDescriptorHandles(const size_t& objectIndex) const;

				CPtr<ID3D12DescriptorHeap> GetVoxelVolumeHeap() const;
				CPtr<ID3D12DescriptorHeap> GetGeometryHeap() const;

				size_t GetMaxObjectsAllowed() const;

			private:
				VDXDescriptorHeap* VoxelVolumeHeap = nullptr;
				VDXDescriptorHeap* GeometryHeap = nullptr;

				size_t MaxObjects;
			};

			class VRDXScene : public VRScene
			{
			public:
				void InitFromScene(std::weak_ptr<VRenderer> renderer, std::weak_ptr<Scene::VScene> scene) override;
				void Cleanup() override;

				void BuildStaticResources(VDXRenderer* renderer);

				D3D12_GPU_VIRTUAL_ADDRESS CopySceneConstantBufferToGPU(const UINT& backBufferIndex);

				void SyncWithScene(std::weak_ptr<VRenderer> renderer, std::weak_ptr<Scene::VScene> scene) override;

				CPtr<ID3D12Resource> GetAccelerationStructureTL() const;
				VDXDescriptorHeap* GetSceneDescriptorHeap() const;
				VDXDescriptorHeap* GetSceneDescriptorHeapSamplers() const;
				CPtr<ID3D12DescriptorHeap> GetGeometrySRVDescriptorHeap() const;
				CPtr<ID3D12DescriptorHeap> GetGeometryCBDescriptorHeap() const;

				CPtr<ID3D12Resource> GetSceneVolume() const;


				void PrepareForRendering(std::weak_ptr<VRenderer> renderer) override;

			private:
				void InitEnvironmentMap(VDXRenderer* renderer);
				void InitSceneGeometry(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene);
				void InitSceneObjects(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene);

				void AllocSceneConstantBuffer(VDXRenderer* renderer);
				void CleanupStaticResources();

				void BuildTopLevelAccelerationStructures(VDXRenderer* renderer);

				void AddVoxelVolume(std::weak_ptr<VDXRenderer> renderer, Voxel::VVoxelVolume* voxelVolume);
				void RemoveVoxelVolume(Voxel::VVoxelVolume* voxelVolume);

				void AddLevelObject(std::weak_ptr<VDXRenderer> renderer, Scene::VLevelObject* levelObject);
				void RemoveLevelObject(Scene::VLevelObject* levelObject);

				void ClearInstanceIDPool();
				void FillInstanceIDPool();

				void UpdateSceneConstantBuffer(std::weak_ptr<Scene::VScene> scene);
				void UpdateSceneGeometry(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene);
				void UpdateSceneObjects(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene);

				bool ContainsLevelObject(Scene::VLevelObject* levelObject) const;

			private:
				VDXDescriptorHeap* DXSceneDescriptorHeap = nullptr;
				VDXDescriptorHeap* DXSceneDescriptorHeapSamplers = nullptr;

				VRDXSceneObjectResourcePool* ObjectResourcePool = nullptr;

				VDXAccelerationStructureBuffers* TLAS = nullptr;

				std::vector<CPtr<ID3D12Resource>> SceneConstantBuffers;
				std::vector<uint8_t*> SceneConstantBufferDataPtrs;

				DirectX::XMMATRIX ViewMatrix;
				DirectX::XMMATRIX ProjectionMatrix;
				DirectX::XMVECTOR CameraPosition;

				float DirectionalLightStrength;
				DirectX::XMFLOAT3 DirectionalLightDirection;

				VObjectPtr<VDXTextureCube> EnvironmentMap = nullptr;

				boost::unordered_map<Voxel::VVoxelVolume*, std::shared_ptr<VDXVoxelVolume>> VoxelVolumes;
				std::vector<std::shared_ptr<VDXLevelObject>> ObjectsInScene;

				size_t NumObjectsInSceneLastFrame = 0;

				boost::lockfree::queue<size_t> GeometryInstancePool{0};

				bool UpdateBLAS = false;
			};
		}
	}
}