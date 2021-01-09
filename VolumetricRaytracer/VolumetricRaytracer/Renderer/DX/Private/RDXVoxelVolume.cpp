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

#include "RDXVoxelVolume.h"
#include "DXTexture3D.h"
#include "Renderer.h"
#include "RaytracingHlsl.h"
#include "VoxelVolume.h"
#include "DXRenderer.h"
#include "TextureFactory.h"

VolumeRaytracer::Renderer::DX::VDXVoxelVolume::VDXVoxelVolume(std::weak_ptr<VDXRenderer> renderer, const VDXVoxelVolumeDesc& volumeDesc)
{
	InitFromVoxelVolume(renderer, volumeDesc);
}

VolumeRaytracer::Renderer::DX::VDXVoxelVolume::~VDXVoxelVolume()
{
	Cleanup();
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::UpdateFromVoxelVolume(std::weak_ptr<VDXRenderer> renderer)
{
	if (!renderer.expired())
	{
		if (LastVoxelCount != Desc.Volume->GetSize())
		{
			UpdateVolumeTexture(renderer);
			UpdateAABBBuffer();
			UpdateGeometryConstantBuffer();
		}
		else
		{
			if (Desc.Volume->IsDirty())
			{
				UpdateVolumeTexture(renderer);
				UpdateGeometryConstantBuffer();
			}

			if (LastCellSize != Desc.Volume->GetCellSize())
			{
				UpdateAABBBuffer();
				UpdateGeometryConstantBuffer();
			}
		}
	}
}

bool VolumeRaytracer::Renderer::DX::VDXVoxelVolume::NeedsUpdate() const
{
	return Desc.Volume->IsDirty();
}

const VolumeRaytracer::Renderer::DX::VDXVoxelVolumeDesc& VolumeRaytracer::Renderer::DX::VDXVoxelVolume::GetDesc() const
{
	return Desc;
}

D3D12_RAYTRACING_GEOMETRY_DESC VolumeRaytracer::Renderer::DX::VDXVoxelVolume::GetGeometryDesc() const
{
	return GeometryDesc;
}

const VolumeRaytracer::Renderer::DX::VDXAccelerationStructureBuffers& VolumeRaytracer::Renderer::DX::VDXVoxelVolume::GetBLAS() const
{
	return BLAS;
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::InitFromVoxelVolume(std::weak_ptr<VDXRenderer> renderer, const VDXVoxelVolumeDesc& volumeDesc)
{
	Desc = volumeDesc;

	AllocateAABBBuffer(renderer);
	AllocateGeometryConstantBuffer(renderer);
	UpdateGeometryDesc();

	UpdateVolumeTexture(renderer);
	UpdateAABBBuffer();
	UpdateGeometryConstantBuffer();

	CreateBottomLevelAccelerationStructure(renderer);
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::Cleanup()
{
	if (AABBBuffer != nullptr)
	{
		AABBBuffer.Reset();
		AABBBuffer = nullptr;
	}

	VolumeTexture = nullptr;
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::CreateBottomLevelAccelerationStructure(std::weak_ptr<VDXRenderer> renderer)
{	
	CPtr<ID3D12Device5> device = renderer.lock()->GetDXDevice();

	CPtr<ID3D12Resource> scratch;
	CPtr<ID3D12Resource> bottomLevelAS;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;

	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.pGeometryDescs = &GeometryDesc;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};

	device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);

	CD3DX12_HEAP_PROPERTIES uavHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC uavResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	device->CreateCommittedResource(&uavHeapProps, D3D12_HEAP_FLAG_NONE, &uavResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&scratch));

	uavHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	uavResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	device->CreateCommittedResource(&uavHeapProps, D3D12_HEAP_FLAG_NONE, &uavResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&bottomLevelAS));

	SetDXDebugName<ID3D12Resource>(bottomLevelAS, "Bottom Level AS");

	bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
	bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAS->GetGPUVirtualAddress();

	BLAS.AccelerationStructure = bottomLevelAS;
	BLAS.Scratch = scratch;
	BLAS.ResultDataMaxSizeInBytes = bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
	BLAS.AccelerationStructureDesc = bottomLevelBuildDesc;
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::AllocateVolumeTexture(std::weak_ptr<VDXRenderer> renderer, const size_t& volumeSize)
{
	if (!renderer.expired())
	{
		size_t pixelCount = volumeSize * 2;

		VolumeTexture = std::static_pointer_cast<VDXTexture3D>(VolumeRaytracer::Renderer::VTextureFactory::CreateTexture3D(renderer, pixelCount, pixelCount, pixelCount, 1));

		renderer.lock()->CreateSRVDescriptor(VolumeTexture, Desc.ResourceHandle);
	}
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::AllocateAABBBuffer(std::weak_ptr<VDXRenderer> renderer)
{
	if (!renderer.expired())
	{
		CD3DX12_HEAP_PROPERTIES uploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_RAYTRACING_AABB));

		CPtr<ID3D12Device5> device = renderer.lock()->GetDXDevice();

		device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&AABBBuffer));
	}
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::AllocateGeometryConstantBuffer(std::weak_ptr<VDXRenderer> renderer)
{
	if (!renderer.expired())
	{
		CPtr<ID3D12Device5> dxDevice = renderer.lock()->GetDXDevice();

		UINT cBufferSize = (sizeof(VGeometryConstantBuffer) + 255) & ~255;

		CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cBufferSize, D3D12_RESOURCE_FLAG_NONE);
		CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&GeometryCB));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cBufferViewDesc = {};

		cBufferViewDesc.BufferLocation = GeometryCB->GetGPUVirtualAddress();
		cBufferViewDesc.SizeInBytes = cBufferSize;

		dxDevice->CreateConstantBufferView(&cBufferViewDesc, Desc.GeometryCBHandle);

		SetDXDebugName<ID3D12Resource>(GeometryCB, "Voxel Geometry Constant Buffer");
	}
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::UpdateVolumeTexture(std::weak_ptr<VDXRenderer> renderer)
{
	std::vector<Voxel::VCellGPUOctreeNode> gpuNodes;
	size_t gpuVolumeSize = 0;

	Desc.Volume->GetGPUOctreeStructure(gpuNodes, gpuVolumeSize);

	if (!VolumeTexture || gpuVolumeSize != LastVolumeSize)
	{
		AllocateVolumeTexture(renderer, gpuVolumeSize);
	}

	if (VolumeTexture && !renderer.expired())
	{
		uint8_t* pixels = nullptr;
		size_t arraySize = 0;

		VolumeTexture->GetPixels(0, pixels, &arraySize);

		size_t voxelCount = Desc.Volume->GetVoxelCount();
		
		VIntVector nodeIndex = VIntVector::ZERO;

		//#pragma omp parallel for
		for (int i = 0; i < gpuNodes.size(); i++)
		{
			const Voxel::VCellGPUOctreeNode& gpuNode = gpuNodes[i];
			nodeIndex = VMathHelpers::Index1DTo3D(i, gpuVolumeSize, gpuVolumeSize);
			nodeIndex = VIntVector(nodeIndex.Z, nodeIndex.X, nodeIndex.Y) * VIntVector(2, 8, 2);

			if (gpuNode.IsLeaf)
			{
				for (int vi = 0; vi < 8; vi++)
				{
					VIntVector relVoxelIndex = Voxel::VCell::VOXEL_COORDS[vi];
					relVoxelIndex = VIntVector(relVoxelIndex.Z, relVoxelIndex.X, relVoxelIndex.Y) * VIntVector(1, 4, 1);

					size_t voxelIndex = VMathHelpers::Index3DTo1D(nodeIndex + relVoxelIndex, gpuVolumeSize * 8, gpuVolumeSize * 2);

					EncodeVoxel(gpuNode.Cell.Voxels[vi], pixels[voxelIndex], pixels[voxelIndex + 1], pixels[voxelIndex + 2]);
					pixels[voxelIndex + 3] = 1;
				}
			}
			else
			{
				for (int ci = 0; ci < 8; ci++)
				{
					VIntVector childIndex3D = gpuNode.Children[ci] * VIntVector(2,2,2);
					childIndex3D = VIntVector(childIndex3D.Z, childIndex3D.X, childIndex3D.Y);

					VIntVector relNodeIndex = Voxel::VCell::VOXEL_COORDS[ci];
					relNodeIndex = VIntVector(relNodeIndex.Z, relNodeIndex.X, relNodeIndex.Y) * VIntVector(1, 4, 1);

					size_t nodePixelIndex = VMathHelpers::Index3DTo1D(nodeIndex + relNodeIndex, gpuVolumeSize * 8, gpuVolumeSize * 2);

					pixels[nodePixelIndex] = (uint8_t)childIndex3D.Y;
					pixels[nodePixelIndex + 1] = (uint8_t)childIndex3D.Z;
					pixels[nodePixelIndex + 2] = (uint8_t)childIndex3D.X;
					pixels[nodePixelIndex + 3] = 0;
				}
			}
		}

		renderer.lock()->UploadToGPU(VolumeTexture);

		LastVoxelCount = Desc.Volume->GetSize();
	}
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::UpdateAABBBuffer()
{
	if (AABBBuffer)
	{
		D3D12_RAYTRACING_AABB dxAABB = {};

		VAABB bounds = Desc.Volume->GetVolumeBounds();

		VVector min = bounds.GetMin();
		VVector max = bounds.GetMax();

		dxAABB.MinX = min.X;
		dxAABB.MinY = min.Y;
		dxAABB.MinZ = min.Z;

		dxAABB.MaxX = max.X;
		dxAABB.MaxY = max.Y;
		dxAABB.MaxZ = max.Z;

		void* mappedData;
		AABBBuffer->Map(0, nullptr, &mappedData);
		memcpy(mappedData, &dxAABB, sizeof(dxAABB));
		AABBBuffer->Unmap(0, nullptr);
		
		LastCellSize = Desc.Volume->GetCellSize();
	}
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::UpdateGeometryDesc()
{
	D3D12_RAYTRACING_GEOMETRY_FLAGS geometryFlags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
	GeometryDesc.AABBs.AABBCount = 1;
	GeometryDesc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
	GeometryDesc.AABBs.AABBs.StartAddress = AABBBuffer->GetGPUVirtualAddress();
	GeometryDesc.Flags = geometryFlags;
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::UpdateGeometryConstantBuffer()
{
	if (GeometryCB != nullptr)
	{
		uint8_t* dataPtr = nullptr;

		CD3DX12_RANGE mapRange(0, 0);
		GeometryCB->Map(0, &mapRange, reinterpret_cast<void**>(&dataPtr));

		VGeometryConstantBuffer constantBufferData = VGeometryConstantBuffer();
		VMaterial volumeMaterial = Desc.Volume->GetMaterial();

		constantBufferData.tint = DirectX::XMFLOAT4(volumeMaterial.AlbedoColor.R, volumeMaterial.AlbedoColor.G, volumeMaterial.AlbedoColor.B, volumeMaterial.AlbedoColor.A);
		constantBufferData.voxelAxisCount = Desc.Volume->GetSize();
		constantBufferData.volumeExtend = Desc.Volume->GetVolumeExtends();
		constantBufferData.distanceBtwVoxels = (constantBufferData.volumeExtend * 2) / (constantBufferData.voxelAxisCount - 1);
		constantBufferData.octreeDepth = Desc.Volume->GetResolution();

		memcpy(dataPtr, &constantBufferData, sizeof(VGeometryConstantBuffer));

		GeometryCB->Unmap(0, nullptr);
	}
}

void VolumeRaytracer::Renderer::DX::VDXVoxelVolume::EncodeVoxel(const Voxel::VVoxel& voxel, uint8_t& outR, uint8_t& outG, uint8_t& outB)
{
	float density = voxel.Density;

	outR = 0;
	outG = 0;
	outB = 0;

	if (density < 0)
	{
		outR = 0x80;
	}

	uint16_t rMask = 0x7f00;
	uint16_t gMask = 0xff;

	uint16_t iDensity = (uint16_t)(std::abs(density) * 100.f);

	outR |= (uint8_t)((iDensity & rMask) >> 8);
	outG = (uint8_t)(iDensity & gMask);

	outB = (uint8_t)voxel.Material;
}
