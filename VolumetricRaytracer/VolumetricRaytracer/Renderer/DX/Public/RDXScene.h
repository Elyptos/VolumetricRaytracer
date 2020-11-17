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

			class VRDXScene : public VRScene
			{
			public:
				void InitFromScene(Voxel::VVoxelScene* scene) override;
				void Cleanup() override;

				void BuildStaticResources(VDXRenderer* renderer);

				D3D12_GPU_VIRTUAL_ADDRESS CopySceneConstantBufferToGPU();

				void SyncWithScene(Voxel::VVoxelScene* scene) override;

				CPtr<ID3D12Resource> GetAccelerationStructureTL() const;

			private:
				void AllocSceneConstantBuffer(VDXRenderer* renderer);
				void BuildGeometryAABB(VDXRenderer* renderer);
				void BuildAccelerationStructure(VDXRenderer* renderer);
				void CleanupStaticResources();

				VDXAccelerationStructureBuffers BuildBottomLevelAccelerationStructures(VDXRenderer* renderer, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc);
				VDXAccelerationStructureBuffers BuildTopLevelAccelerationStructures(VDXRenderer* renderer, const VDXAccelerationStructureBuffers& bottomLevelAS);

				D3D12_RAYTRACING_GEOMETRY_DESC CreateGeometryDesc();

			private:
				CPtr<ID3D12DescriptorHeap> ResourceDescHeap = nullptr;
				unsigned int ResourceDescHeapSize = 0;
				unsigned int NumResourceDescriptors = 0;
				unsigned int ResourceDescSize = 0;

				CPtr<ID3D12Resource> VoxelVolumeTexture;
				D3D12_GPU_DESCRIPTOR_HANDLE VoxelVolumeTextureGPUHandle;

				VD3DBuffer AABBBuffer;

				CPtr<ID3D12Resource> BottomLevelAS = nullptr;
				CPtr<ID3D12Resource> TopLevelAS = nullptr;

				CPtr<ID3D12Resource> SceneConstantBuffer;
				uint8_t* SceneConstantBufferDataPtr;

				DirectX::XMMATRIX ViewMatrix;
				DirectX::XMMATRIX ProjectionMatrix;
				DirectX::XMVECTOR CameraPosition;
			};
		}
	}
}