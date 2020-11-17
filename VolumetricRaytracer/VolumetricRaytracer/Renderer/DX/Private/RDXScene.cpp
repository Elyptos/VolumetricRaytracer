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
#include <DirectXMath.h>

void VolumeRaytracer::Renderer::DX::VRDXScene::InitFromScene(Voxel::VVoxelScene* scene)
{
	VRScene::InitFromScene(scene);
}

void VolumeRaytracer::Renderer::DX::VRDXScene::Cleanup()
{
	VRScene::Cleanup();
}

void VolumeRaytracer::Renderer::DX::VRDXScene::BuildStaticResources(VDXRenderer* renderer)
{
	CleanupStaticResources();
	AllocSceneConstantBuffer(renderer);
	BuildGeometryAABB(renderer);
	BuildAccelerationStructure(renderer);
}

D3D12_GPU_VIRTUAL_ADDRESS VolumeRaytracer::Renderer::DX::VRDXScene::CopySceneConstantBufferToGPU()
{
	VSceneConstantBuffer constantBufferData = VSceneConstantBuffer();

	constantBufferData.cameraPosition = CameraPosition;
	constantBufferData.projectionToWorld = XMMatrixInverse(nullptr, ViewMatrix * ProjectionMatrix);

	memcpy(SceneConstantBufferDataPtr, &constantBufferData, sizeof(VSceneConstantBuffer));

	return SceneConstantBuffer->GetGPUVirtualAddress();
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

	ViewMatrix = DirectX::XMMatrixLookAtLH(CameraPosition, lookAtVecDX, upVecDX);
	ProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(cam->FOVAngle), cam->AspectRatio, cam->NearClipPlane, cam->FarClipPlane);
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VRDXScene::GetAccelerationStructureTL() const
{
	return TopLevelAS;
}

void VolumeRaytracer::Renderer::DX::VRDXScene::AllocSceneConstantBuffer(VDXRenderer* renderer)
{
	CPtr<ID3D12Device5> dxDevice = renderer->GetDXDevice();

	UINT cBufferSize = (sizeof(VSceneConstantBuffer) + 255) & ~255; 

	CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cBufferSize, D3D12_RESOURCE_FLAG_NONE);
	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&SceneConstantBuffer));

	CD3DX12_RANGE mapRange(0, 0);
	SceneConstantBuffer->Map(0, &mapRange, reinterpret_cast<void**>(&SceneConstantBufferDataPtr));
}

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
	if (SceneConstantBuffer != nullptr)
	{
		SceneConstantBuffer->Unmap(0, nullptr);
		SceneConstantBuffer.Reset();
		SceneConstantBuffer = nullptr;
	}

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

	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};

	instanceDesc.InstanceMask = 1;

	//@TODO: Set hit group here
	instanceDesc.InstanceContributionToHitGroupIndex = 2;
	instanceDesc.AccelerationStructure = bottomLevelAS.AccelerationStructure->GetGPUVirtualAddress();
	
	CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(instanceDesc));

	device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&instanceDescsResource));

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
