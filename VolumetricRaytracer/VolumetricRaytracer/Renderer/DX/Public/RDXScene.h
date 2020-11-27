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
#include <vector>
#include <DirectXMath.h>

struct D3D12_GPU_DESCRIPTOR_HANDLE;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct ID3D12Device5;

namespace VolumeRaytracer
{
	namespace Renderer
	{
		namespace DX
		{
			class VDXRenderer;
			class VDXDescriptorHeap;
			class VDXTextureCube;

			class VRDXScene : public VRScene
			{
			public:
				void InitFromScene(Voxel::VVoxelScene* scene) override;
				void Cleanup() override;

				void BuildStaticResources(VDXRenderer* renderer);

				D3D12_GPU_VIRTUAL_ADDRESS CopySceneConstantBufferToGPU(const UINT& backBufferIndex);

				void SyncWithScene(Voxel::VVoxelScene* scene) override;

				CPtr<ID3D12Resource> GetAccelerationStructureTL() const;
				VDXDescriptorHeap* GetSceneDescriptorHeap() const;
				VDXDescriptorHeap* GetSceneDescriptorHeapSamplers() const;

				CPtr<ID3D12Resource> GetSceneVolume() const;

				void BuildVoxelVolume(CPtr<ID3D12GraphicsCommandList5> commandList);

			private:
				void InitEnvironmentMap(VDXRenderer* renderer);
				void AllocSceneConstantBuffer(VDXRenderer* renderer);
				void AllocSceneVolumeBuffer(VDXRenderer* renderer);
				void BuildGeometryAABB(VDXRenderer* renderer);
				void BuildAccelerationStructure(VDXRenderer* renderer);
				void CleanupStaticResources();

				void PrepareVoxelVolume(Voxel::VVoxelScene* scene);

				VDXAccelerationStructureBuffers BuildBottomLevelAccelerationStructures(VDXRenderer* renderer, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
				VDXAccelerationStructureBuffers BuildTopLevelAccelerationStructures(VDXRenderer* renderer, const VDXAccelerationStructureBuffers& bottomLevelAS);

				D3D12_RAYTRACING_GEOMETRY_DESC CreateGeometryDesc();

			private:
				VDXDescriptorHeap* DXDescriptorHeap = nullptr;
				VDXDescriptorHeap* DXDescriptorHeapSamplers = nullptr;

				VD3DBuffer AABBBuffer;

				CPtr<ID3D12Resource> BottomLevelAS = nullptr;
				CPtr<ID3D12Resource> TopLevelAS = nullptr;

				std::vector<CPtr<ID3D12Resource>> SceneConstantBuffers;
				std::vector<uint8_t*> SceneConstantBufferDataPtrs;

				CPtr<ID3D12Resource> SceneVolume;
				CPtr<ID3D12Resource> SceneVolumeUploadBuffer;
				
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT UploadBufferFootprint;
				D3D12_GPU_DESCRIPTOR_HANDLE SceneVolumeTextureGPUHandle;

				DirectX::XMMATRIX ViewMatrix;
				DirectX::XMMATRIX ProjectionMatrix;
				DirectX::XMVECTOR CameraPosition;

				VObjectPtr<VDXTextureCube> EnvironmentMap = nullptr;
			};
		}
	}
}