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

#include "DXHelper.h"
#include "Object.h"
#include <memory>

namespace VolumeRaytracer
{
	namespace Voxel
	{
		class VVoxelVolume;
		class VVoxel;
	}

	namespace Renderer
	{
		namespace DX
		{
			class VDXTexture3D;
			class VDXRenderer;

			struct VDXVoxelVolumeDesc
			{
			public:
				Voxel::VVoxelVolume* Volume;
				D3D12_CPU_DESCRIPTOR_HANDLE VolumeHandle;
				D3D12_CPU_DESCRIPTOR_HANDLE TraversalHandle;
				D3D12_CPU_DESCRIPTOR_HANDLE GeometryCBHandle;
				size_t InstanceIndex;
			};

			struct VDXVoxelVolumeTextureIndices
			{
			public:
				size_t AlbedoIndex = 0;
				size_t NormalIndex = 1;
				size_t RMIndex = 2;
			};

			class VDXVoxelVolume
			{
			public:
				VDXVoxelVolume(std::weak_ptr<VDXRenderer> renderer, const VDXVoxelVolumeDesc& volumeDesc);
				~VDXVoxelVolume();

				void UpdateFromVoxelVolume(std::weak_ptr<VDXRenderer> renderer);
				void SetTextures(const VDXVoxelVolumeTextureIndices& textureIndices);
				bool NeedsUpdate() const;

				const VDXVoxelVolumeDesc& GetDesc() const;
				D3D12_RAYTRACING_GEOMETRY_DESC GetGeometryDesc() const;

				const VDXAccelerationStructureBuffers& GetBLAS() const;

			private:
				void InitFromVoxelVolume(std::weak_ptr<VDXRenderer> renderer, const VDXVoxelVolumeDesc& volumeDesc);
				void Cleanup();

				void CreateBottomLevelAccelerationStructure(std::weak_ptr<VDXRenderer> renderer);
				void AllocateTraversalTexture(std::weak_ptr<VDXRenderer> renderer, const size_t& traversalNodeCount);
				void AllocateVolumeTexture(std::weak_ptr<VDXRenderer> renderer, const size_t& volumeSize);
				void AllocateAABBBuffer(std::weak_ptr<VDXRenderer> renderer);
				void AllocateGeometryConstantBuffer(std::weak_ptr<VDXRenderer> renderer);

				void UpdateTraversalTexture(std::weak_ptr<VDXRenderer> renderer);
				void UpdateVolumeTexture(std::weak_ptr<VDXRenderer> renderer);
				void UpdateAABBBuffer();
				void UpdateGeometryDesc();
				void UpdateGeometryConstantBuffer();

				void EncodeVoxel(const Voxel::VVoxel& voxel, uint8_t& outR, uint8_t& outG, uint8_t& outB);

			private:
				VObjectPtr<VDXTexture3D> VolumeTexture = nullptr;
				VObjectPtr<VDXTexture3D> TraversalTexture = nullptr;
				CPtr<ID3D12Resource> AABBBuffer;
				CPtr<ID3D12Resource> GeometryCB;
				
				VDXAccelerationStructureBuffers BLAS;

				VDXVoxelVolumeDesc Desc;
				VDXVoxelVolumeTextureIndices TextureIndices;
				size_t LastVoxelCount;
				size_t LastTraversalNodeCount;
				float LastCellSize;

				D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc;
			};
		}
	}
}