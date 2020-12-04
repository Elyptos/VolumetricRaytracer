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

#include "RDXScene.h"
#include "DXRenderer.h"
#include "RaytracingHlsl.h"
#include "Camera.h"
#include "DXDescriptorHeap.h"
#include <DirectXMath.h>
#include "DXTextureCube.h"
#include "DXConstants.h"
#include "TextureFactory.h"
#include "DXTexture3D.h"

void VolumeRaytracer::Renderer::DX::VRDXScene::InitFromScene(Voxel::VVoxelScene* scene)
{
	VRScene::InitFromScene(scene);

	EnvironmentMap = std::static_pointer_cast<VDXTextureCube>(scene->GetEnvironmentTexture());
}

void VolumeRaytracer::Renderer::DX::VRDXScene::Cleanup()
{
	VRScene::Cleanup();
	CleanupStaticResources();

	SceneVolume = nullptr;
}

void VolumeRaytracer::Renderer::DX::VRDXScene::BuildStaticResources(VDXRenderer* renderer)
{
	CleanupStaticResources();

	if (DXDescriptorHeap == nullptr)
	{
		DXDescriptorHeap = new VDXDescriptorHeap(renderer->GetDXDevice(), 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}

	if (DXDescriptorHeapSamplers == nullptr)
	{
		DXDescriptorHeapSamplers = new VDXDescriptorHeap(renderer->GetDXDevice(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}

	AllocSceneConstantBuffer(renderer);
	//AllocSceneVolumeBuffer(renderer);
	BuildGeometryAABB(renderer);
	BuildAccelerationStructure(renderer);

	InitEnvironmentMap(renderer);
}

D3D12_GPU_VIRTUAL_ADDRESS VolumeRaytracer::Renderer::DX::VRDXScene::CopySceneConstantBufferToGPU(const UINT& backBufferIndex)
{
	VSceneConstantBuffer constantBufferData = VSceneConstantBuffer();

	constantBufferData.cameraPosition = CameraPosition;
	constantBufferData.voxelAxisCount = VoxelCountAlongAxis;
	constantBufferData.volumeExtend = VolumeExtends;
	constantBufferData.distanceBtwVoxels = (VolumeExtends * 2) / VoxelCountAlongAxis;

	XMVECTOR det;
	constantBufferData.viewMatrixInverted = XMMatrixInverse(&det, ViewMatrix);
	constantBufferData.projectionMatrixInverted = XMMatrixInverse(&det, ProjectionMatrix);

	memcpy(SceneConstantBufferDataPtrs[backBufferIndex], &constantBufferData, sizeof(VSceneConstantBuffer));

	return SceneConstantBuffers[backBufferIndex]->GetGPUVirtualAddress();
}

void VolumeRaytracer::Renderer::DX::VRDXScene::SyncWithScene(Voxel::VVoxelScene* scene)
{
	VRScene::SyncWithScene(scene);

	VObjectPtr<Scene::VCamera> cam = scene->GetSceneCamera();

	VVector lookAtVec = cam->Rotation.GetForwardVector();
	VVector upVec = cam->Rotation.GetUpVector();

	DirectX::XMVECTOR lookAtVecDX = DirectX::XMVectorSet(lookAtVec.X, lookAtVec.Y, lookAtVec.Z, 1.f);
	DirectX::XMVECTOR upVecDX = DirectX::XMVectorSet(upVec.X, upVec.Y, upVec.Z, 1.f);

	CameraPosition = DirectX::XMVectorSet(cam->Position.X, cam->Position.Y, cam->Position.Z, 1.f);

	ViewMatrix = DirectX::XMMatrixLookToRH(CameraPosition, lookAtVecDX, upVecDX);
	ProjectionMatrix = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(cam->FOVAngle), cam->AspectRatio, cam->NearClipPlane, cam->FarClipPlane);

	//PrepareVoxelVolume(scene);
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VRDXScene::GetAccelerationStructureTL() const
{
	return TopLevelAS;
}

VolumeRaytracer::Renderer::DX::VDXDescriptorHeap* VolumeRaytracer::Renderer::DX::VRDXScene::GetSceneDescriptorHeap() const
{
	return DXDescriptorHeap;
}

VolumeRaytracer::Renderer::DX::VDXDescriptorHeap* VolumeRaytracer::Renderer::DX::VRDXScene::GetSceneDescriptorHeapSamplers() const
{
	return DXDescriptorHeapSamplers;
}

void VolumeRaytracer::Renderer::DX::VRDXScene::BuildVoxelVolume(Voxel::VVoxelScene* scene, std::weak_ptr<VDXRenderer> renderer)
{
	SceneVolume = std::static_pointer_cast<VDXTexture3D>(VolumeRaytracer::Renderer::VTextureFactory::CreateTexture3D(renderer, scene->GetSize(), scene->GetSize(), scene->GetSize(), 1));

	std::shared_ptr<VDXRenderer> rendererPtr = renderer.lock();

	uint8_t* pixels = nullptr;
	size_t arraySize = 0;

	SceneVolume->GetPixels(0, pixels, &arraySize);

	for (const Voxel::VVoxelIteratorElement& voxel : *scene)
	{
		UINT rIndex = voxel.Index * 4;
		pixels[rIndex] = voxel.Voxel.Material > 0 ? 255 : 0;
		pixels[rIndex + 1] = 0;
		pixels[rIndex + 2] = 0;
		pixels[rIndex + 3] = 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	UINT descIndex = 0;

	DXDescriptorHeap->AllocateDescriptor(&cpuHandle, &gpuHandle, descIndex);

	rendererPtr->CreateSRVDescriptor(SceneVolume, cpuHandle);
	rendererPtr->UploadToGPU(SceneVolume);
}

void VolumeRaytracer::Renderer::DX::VRDXScene::InitEnvironmentMap(VDXRenderer* renderer)
{
	if (EnvironmentMap != nullptr)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		UINT descIndex = 0;

		DXDescriptorHeap->AllocateDescriptor(&cpuHandle, &gpuHandle, descIndex);

		renderer->CreateSRVDescriptor(EnvironmentMap, cpuHandle);

		renderer->UploadToGPU(EnvironmentMap);
		
		DXDescriptorHeapSamplers->AllocateDescriptor(&cpuHandle, &gpuHandle, descIndex);

		D3D12_SAMPLER_DESC samplerDesc = {};

		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;

		renderer->GetDXDevice()->CreateSampler(&samplerDesc, cpuHandle);
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::AllocSceneConstantBuffer(VDXRenderer* renderer)
{
	CPtr<ID3D12Device5> dxDevice = renderer->GetDXDevice();

	UINT cBufferSize = (sizeof(VSceneConstantBuffer) + 255) & ~255;

	for (UINT i = 0; i < VDXConstants::BACK_BUFFER_COUNT; i++)
	{
		SceneConstantBuffers.push_back(nullptr);
		SceneConstantBufferDataPtrs.push_back(nullptr);

		CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cBufferSize, D3D12_RESOURCE_FLAG_NONE);
		CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&SceneConstantBuffers[i]));

		SetDXDebugName<ID3D12Resource>(SceneConstantBuffers[i], "Voxel Scene Constant Buffer");

		uint8_t* dataPtr = nullptr;

		CD3DX12_RANGE mapRange(0, 0);
		SceneConstantBuffers[i]->Map(0, &mapRange, reinterpret_cast<void**>(&dataPtr));

		SceneConstantBufferDataPtrs[i] = dataPtr;
	}
}

//void VolumeRaytracer::Renderer::DX::VRDXScene::AllocSceneVolumeBuffer(VDXRenderer* renderer)
//{
//	CPtr<ID3D12Device5> dxDevice = renderer->GetDXDevice();
//
//	CD3DX12_RESOURCE_DESC volumeBufferDesc = CD3DX12_RESOURCE_DESC::Tex3D(DXGI_FORMAT_R8G8B8A8_UNORM, VoxelCountAlongAxis, VoxelCountAlongAxis, VoxelCountAlongAxis, 1, 
//	D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_UNKNOWN);
//
//	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
//
//	dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &volumeBufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&SceneVolume));
//
//	SetDXDebugName<ID3D12Resource>(SceneVolume, "Voxel Volume Texture");
//
//	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
//	UINT allocaationIndex;
//
//	if (DXDescriptorHeap->AllocateDescriptor(&cpuHandle, &SceneVolumeTextureGPUHandle, allocaationIndex))
//	{
//		D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
//
//		shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
//		shaderResourceViewDesc.Texture3D.MipLevels = 1;
//		shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//
//		dxDevice->CreateShaderResourceView(SceneVolume.Get(), &shaderResourceViewDesc, cpuHandle);
//	}
//
//	UINT64 uploadBufferSize = GetRequiredIntermediateSize(SceneVolume.Get(), 0, 1);
//
//	CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize, D3D12_RESOURCE_FLAG_NONE);
//	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//
//	dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&SceneVolumeUploadBuffer));
//
//	SetDXDebugName<ID3D12Resource>(SceneVolumeUploadBuffer, "Voxel Volume Upload Buffer");
//}

void VolumeRaytracer::Renderer::DX::VRDXScene::BuildGeometryAABB(VDXRenderer* renderer)
{
	D3D12_RAYTRACING_AABB dxAABB = {};

	VVector min = Bounds.GetMin();
	VVector max = Bounds.GetMax();

	dxAABB.MinX = min.X;
	dxAABB.MinY = min.Y;
	dxAABB.MinZ = min.Z;

	dxAABB.MaxX = max.X;
	dxAABB.MaxY = max.Y;
	dxAABB.MaxZ = max.Z;

	CD3DX12_HEAP_PROPERTIES uploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(dxAABB));

	CPtr<ID3D12Device5> device = renderer->GetDXDevice();

	device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&AABBBuffer.Resource));

	void* mappedData;
	AABBBuffer.Resource->Map(0, nullptr, &mappedData);
	memcpy(mappedData, &dxAABB, sizeof(dxAABB));
	AABBBuffer.Resource->Unmap(0, nullptr);
}

void VolumeRaytracer::Renderer::DX::VRDXScene::BuildAccelerationStructure(VDXRenderer* renderer)
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = CreateGeometryDesc();

	VDXAccelerationStructureBuffers bottomLevelAS = BuildBottomLevelAccelerationStructures(renderer, geometryDesc);
	VDXAccelerationStructureBuffers topLevelAS = BuildTopLevelAccelerationStructures(renderer, bottomLevelAS);

	renderer->BuildAccelerationStructure(topLevelAS, bottomLevelAS);

	BottomLevelAS = bottomLevelAS.AccelerationStructure;
	TopLevelAS = topLevelAS.AccelerationStructure;
}

void VolumeRaytracer::Renderer::DX::VRDXScene::CleanupStaticResources()
{
	//if (SceneVolume != nullptr)
	//{
	//	SceneVolume.Reset();
	//	SceneVolume = nullptr;
	//}

	//if (SceneVolumeUploadBuffer != nullptr)
	//{
	//	SceneVolumeUploadBuffer.Reset();
	//	SceneVolumeUploadBuffer = nullptr;
	//}

	if (DXDescriptorHeapSamplers != nullptr)
	{
		delete DXDescriptorHeapSamplers;
		DXDescriptorHeapSamplers = nullptr;
	}

	if (DXDescriptorHeap != nullptr)
	{
		delete DXDescriptorHeap;
		DXDescriptorHeap = nullptr;
	}

	for (auto& constantBuffer : SceneConstantBuffers)
	{
		if (constantBuffer != nullptr)
		{
			constantBuffer->Unmap(0, nullptr);
			constantBuffer.Reset();
			constantBuffer = nullptr;
		}
	}

	SceneConstantBuffers.empty();
	SceneConstantBufferDataPtrs.empty();

	if (TopLevelAS != nullptr)
	{
		TopLevelAS.Reset();
		TopLevelAS = nullptr;
	}
	
	if (BottomLevelAS != nullptr)
	{
		BottomLevelAS.Reset();
		BottomLevelAS = nullptr;
	}

	AABBBuffer.Release();
}

//void VolumeRaytracer::Renderer::DX::VRDXScene::PrepareVoxelVolume(Voxel::VVoxelScene* scene)
//{
//	UINT8* rawSceneData = new UINT8[4 * scene->GetVoxelCount()];
//
//	for (const Voxel::VVoxelIteratorElement& voxel : *scene)
//	{
//		UINT rIndex = voxel.Index * 4;
//		rawSceneData[rIndex] = voxel.Voxel.Material > 0 ? 255 : 0;
//		rawSceneData[rIndex + 1] = 0;
//		rawSceneData[rIndex + 2] = 0;
//		rawSceneData[rIndex + 3] = 0;
//	}
//
//	D3D12_SUBRESOURCE_FOOTPRINT footprint = {};
//	footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	footprint.Width = VoxelCountAlongAxis;
//	footprint.Height = VoxelCountAlongAxis;
//	footprint.Depth = VoxelCountAlongAxis;
//	footprint.RowPitch = VDXHelper::Align(VoxelCountAlongAxis * sizeof(DWORD), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
//
//	uint8_t* uploadBufferBegin;
//
//	CD3DX12_RANGE mapRange(0, 0);
//	SceneVolumeUploadBuffer->Map(0, &mapRange, reinterpret_cast<void**>(&uploadBufferBegin));
//
//	uint8_t* uploadBufferEnd = uploadBufferBegin + GetRequiredIntermediateSize(SceneVolume.Get(), 0, 1);
//	//UINT8* uploadBufferCur = reinterpret_cast<UINT8*>(VDXHelper::Align(reinterpret_cast<SIZE_T>(uploadBufferBegin), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));
//
//	UploadBufferFootprint = {};
//
//	UploadBufferFootprint.Offset = 0;
//	UploadBufferFootprint.Footprint = footprint;
//
//	for (UINT y = 0; y < VoxelCountAlongAxis; y++)
//	{
//		for (UINT z = 0; z < VoxelCountAlongAxis; z++)
//		{
//			uint8_t* dest = uploadBufferBegin + UploadBufferFootprint.Offset + z * VoxelCountAlongAxis * footprint.RowPitch + y * footprint.RowPitch;
//			uint8_t* source = &rawSceneData[y * VoxelCountAlongAxis + z * VoxelCountAlongAxis * VoxelCountAlongAxis];
//
//			memcpy(dest, source, sizeof(DWORD) * VoxelCountAlongAxis);
//		}
//	}
//
//	SceneVolumeUploadBuffer->Unmap(0, nullptr);
//
//	delete[] rawSceneData;
//}

//void VolumeRaytracer::Renderer::DX::VRDXScene::BuildVoxelVolume(CPtr<ID3D12GraphicsCommandList5> commandList)
//{
//	commandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(SceneVolume.Get(), 0), 0, 0, 0, &CD3DX12_TEXTURE_COPY_LOCATION(SceneVolumeUploadBuffer.Get(), UploadBufferFootprint), nullptr);
//}

VolumeRaytracer::Renderer::DX::VDXAccelerationStructureBuffers VolumeRaytracer::Renderer::DX::VRDXScene::BuildBottomLevelAccelerationStructures(VDXRenderer* renderer, D3D12_RAYTRACING_GEOMETRY_DESC& geometryDesc)
{
	CPtr<ID3D12Device5> device = renderer->GetDXDevice();

	VDXAccelerationStructureBuffers bottomLevelASBuffers;

	CPtr<ID3D12Resource> scratch;
	CPtr<ID3D12Resource> bottomLevelAS;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;

	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.pGeometryDescs = &geometryDesc;

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

	bottomLevelASBuffers.AccelerationStructure = bottomLevelAS;
	bottomLevelASBuffers.Scratch = scratch;
	bottomLevelASBuffers.ResultDataMaxSizeInBytes = bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes;
	bottomLevelASBuffers.AccelerationStructureDesc = bottomLevelBuildDesc;

	return bottomLevelASBuffers;
}

VolumeRaytracer::Renderer::DX::VDXAccelerationStructureBuffers VolumeRaytracer::Renderer::DX::VRDXScene::BuildTopLevelAccelerationStructures(VDXRenderer* renderer, const VDXAccelerationStructureBuffers& bottomLevelAS)
{
	CPtr<ID3D12Resource> scratch;
	CPtr<ID3D12Resource> topLevelAS;
	CPtr<ID3D12Resource> instanceDescsResource;

	CPtr<ID3D12Device5> device = renderer->GetDXDevice();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.NumDescs = 1;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

	CD3DX12_HEAP_PROPERTIES uavHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC uavResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	device->CreateCommittedResource(&uavHeapProps, D3D12_HEAP_FLAG_NONE, &uavResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&scratch));

	uavHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	uavResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	device->CreateCommittedResource(&uavHeapProps, D3D12_HEAP_FLAG_NONE, &uavResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&topLevelAS));
	SetDXDebugName<ID3D12Resource>(topLevelAS, "Top Level AS");

	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};

	instanceDesc.InstanceMask = 1;

	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.AccelerationStructure = bottomLevelAS.AccelerationStructure->GetGPUVirtualAddress();
	
	DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(1.f, 1.f, 1.f);
	DirectX::XMMATRIX translation = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f));
	DirectX::XMMATRIX transform = scale * translation;

	DirectX::XMStoreFloat3x4(reinterpret_cast<DirectX::XMFLOAT3X4*>(instanceDesc.Transform), transform);

	CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(instanceDesc));

	device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&instanceDescsResource));

	SetDXDebugName<ID3D12Resource>(instanceDescsResource, "Instance Description resource");

	void* mappedData;
	instanceDescsResource->Map(0, nullptr, &mappedData);
	memcpy(mappedData, &instanceDesc, sizeof(instanceDesc));
	instanceDescsResource->Unmap(0, nullptr);

	topLevelBuildDesc.DestAccelerationStructureData = topLevelAS->GetGPUVirtualAddress();
	topLevelInputs.InstanceDescs = instanceDescsResource->GetGPUVirtualAddress();
	topLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();

	VDXAccelerationStructureBuffers topLevelASStructure;

	topLevelASStructure.Scratch = scratch;
	topLevelASStructure.AccelerationStructureDesc = topLevelBuildDesc;
	topLevelASStructure.AccelerationStructure = topLevelAS;
	topLevelASStructure.InstanceDesc = instanceDescsResource;
	topLevelASStructure.ResultDataMaxSizeInBytes = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;

	return topLevelASStructure;
}

D3D12_RAYTRACING_GEOMETRY_DESC VolumeRaytracer::Renderer::DX::VRDXScene::CreateGeometryDesc()
{
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	D3D12_RAYTRACING_GEOMETRY_FLAGS geometryFlags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
	geometryDesc.AABBs.AABBCount = 1;
	geometryDesc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
	geometryDesc.AABBs.AABBs.StartAddress = AABBBuffer.Resource->GetGPUVirtualAddress();
	geometryDesc.Flags = geometryFlags;

	return geometryDesc;
}
