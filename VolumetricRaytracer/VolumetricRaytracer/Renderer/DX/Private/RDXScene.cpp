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
#include "Light.h"
#include "DXDescriptorHeap.h"
#include <DirectXMath.h>
#include "DXTextureCube.h"
#include "DXConstants.h"
#include "TextureFactory.h"
#include "DXTexture3DFloat.h"
#include "RDXVoxelVolume.h"
#include "RDXLevelObject.h"
#include "RenderableObject.h"
#include "Logger.h"

void VolumeRaytracer::Renderer::DX::VRDXScene::InitFromScene(std::weak_ptr<VRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VRScene::InitFromScene(renderer, scene);
	EnvironmentMap = std::static_pointer_cast<VDXTextureCube>(scene.lock()->GetEnvironmentTexture());

	VObjectPtr<VDXRenderer> dxRenderer = std::static_pointer_cast<VDXRenderer>(renderer.lock());
	BuildStaticResources(dxRenderer.get());

	InitSceneGeometry(dxRenderer, scene);
	InitSceneObjects(dxRenderer, scene);
}

void VolumeRaytracer::Renderer::DX::VRDXScene::Cleanup()
{
	VRScene::Cleanup();
	CleanupStaticResources();
}

void VolumeRaytracer::Renderer::DX::VRDXScene::BuildStaticResources(VDXRenderer* renderer)
{
	CleanupStaticResources();

	FillInstanceIDPool();

	if (DXSceneDescriptorHeap == nullptr)
	{
		DXSceneDescriptorHeap = new VDXDescriptorHeap(renderer->GetDXDevice(), 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}

	if (DXSceneDescriptorHeapSamplers == nullptr)
	{
		DXSceneDescriptorHeapSamplers = new VDXDescriptorHeap(renderer->GetDXDevice(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}

	if (ObjectResourcePool == nullptr)
	{
		ObjectResourcePool = new VRDXSceneObjectResourcePool(renderer->GetDXDevice(), MaxAllowedObjectData);
	}

	AllocSceneConstantBuffer(renderer);
	InitEnvironmentMap(renderer);
}

D3D12_GPU_VIRTUAL_ADDRESS VolumeRaytracer::Renderer::DX::VRDXScene::CopySceneConstantBufferToGPU(const UINT& backBufferIndex)
{
	VSceneConstantBuffer constantBufferData = VSceneConstantBuffer();

	constantBufferData.cameraPosition = CameraPosition;

	XMVECTOR det;
	constantBufferData.viewMatrixInverted = XMMatrixInverse(&det, ViewMatrix);
	constantBufferData.projectionMatrixInverted = XMMatrixInverse(&det, ProjectionMatrix);

	constantBufferData.dirLightDirection = DirectionalLightDirection;
	constantBufferData.dirLightStrength = DirectionalLightStrength;

	memcpy(SceneConstantBufferDataPtrs[backBufferIndex], &constantBufferData, sizeof(VSceneConstantBuffer));

	return SceneConstantBuffers[backBufferIndex]->GetGPUVirtualAddress();
}

void VolumeRaytracer::Renderer::DX::VRDXScene::SyncWithScene(std::weak_ptr<VRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VRScene::SyncWithScene(renderer, scene);
	
	std::shared_ptr<VDXRenderer> dxRenderer = std::static_pointer_cast<VDXRenderer>(renderer.lock());

	UpdateSceneConstantBuffer(scene);
	UpdateSceneGeometry(dxRenderer, scene);
	UpdateSceneObjects(dxRenderer, scene);
}

VolumeRaytracer::Renderer::DX::VDXAccelerationStructureBuffers* VolumeRaytracer::Renderer::DX::VRDXScene::GetAccelerationStructureTL(const unsigned int& backBufferIndex) const
{
	return TLAS[backBufferIndex];
}

VolumeRaytracer::Renderer::DX::VDXDescriptorHeap* VolumeRaytracer::Renderer::DX::VRDXScene::GetSceneDescriptorHeap() const
{
	return DXSceneDescriptorHeap;
}

VolumeRaytracer::Renderer::DX::VDXDescriptorHeap* VolumeRaytracer::Renderer::DX::VRDXScene::GetSceneDescriptorHeapSamplers() const
{
	return DXSceneDescriptorHeapSamplers;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12DescriptorHeap> VolumeRaytracer::Renderer::DX::VRDXScene::GetGeometrySRVDescriptorHeap() const
{
	return ObjectResourcePool->GetVoxelVolumeHeap();
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12DescriptorHeap> VolumeRaytracer::Renderer::DX::VRDXScene::GetGeometryCBDescriptorHeap() const
{
	return ObjectResourcePool->GetGeometryHeap();
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12DescriptorHeap> VolumeRaytracer::Renderer::DX::VRDXScene::GetGeometryTraversalDescriptorHeap() const
{
	return ObjectResourcePool->GetGeometryTraversalHeap();
}

void VolumeRaytracer::Renderer::DX::VRDXScene::PrepareForRendering(std::weak_ptr<VRenderer> renderer, const unsigned int& backBufferIndex)
{
	VObjectPtr<VDXRenderer> dxRenderer = std::static_pointer_cast<VDXRenderer>(renderer.lock());

	BuildTopLevelAccelerationStructures(dxRenderer.get(), backBufferIndex);

	if(TLAS.size() <= backBufferIndex)
		return;

	if (UpdateBLAS)
	{
		UpdateBLAS = false;

		std::vector<VDXAccelerationStructureBuffers> blas;

		for (auto& elem : VoxelVolumes)
		{
			blas.push_back(elem.second->GetBLAS());
		}

		dxRenderer->BuildBottomLevelAccelerationStructure(blas);
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::InitEnvironmentMap(VDXRenderer* renderer)
{
	if (EnvironmentMap != nullptr)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		UINT descIndex = 0;

		DXSceneDescriptorHeap->AllocateDescriptor(&cpuHandle, &gpuHandle, descIndex);

		renderer->CreateSRVDescriptor(EnvironmentMap, cpuHandle);

		renderer->UploadToGPU(EnvironmentMap);
		
		DXSceneDescriptorHeapSamplers->AllocateDescriptor(&cpuHandle, &gpuHandle, descIndex);

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

void VolumeRaytracer::Renderer::DX::VRDXScene::InitSceneGeometry(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VObjectPtr<Scene::VScene> scenePtr = scene.lock();

	std::vector<std::weak_ptr<Voxel::VVoxelVolume>> volumes = scenePtr->GetAllRegisteredVolumes();

	for (auto& elem : volumes)
	{
		AddVoxelVolume(renderer, elem.lock().get());
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::InitSceneObjects(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VObjectPtr<Scene::VScene> scenePtr = scene.lock();

	std::vector<std::weak_ptr<Scene::VLevelObject>> objects = scenePtr->GetAllPlacedObjects();

	for (auto& elem : objects)
	{
		AddLevelObject(renderer, elem.lock().get());
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::AllocSceneConstantBuffer(VDXRenderer* renderer)
{
	CPtr<ID3D12Device5> dxDevice = renderer->GetDXDevice();

	UINT rawSize = sizeof(VSceneConstantBuffer);

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

void VolumeRaytracer::Renderer::DX::VRDXScene::CleanupStaticResources()
{
	if (DXSceneDescriptorHeapSamplers != nullptr)
	{
		delete DXSceneDescriptorHeapSamplers;
		DXSceneDescriptorHeapSamplers = nullptr;
	}

	if (DXSceneDescriptorHeap != nullptr)
	{
		delete DXSceneDescriptorHeap;
		DXSceneDescriptorHeap = nullptr;
	}

	if (ObjectResourcePool != nullptr)
	{
		delete ObjectResourcePool;
		ObjectResourcePool = nullptr;
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

	SceneConstantBuffers.clear();
	SceneConstantBufferDataPtrs.clear();

	for (auto& tlas : TLAS)
	{
		if (tlas != nullptr)
		{
			delete tlas;
			tlas = nullptr;
		}
	}

	TLAS.clear();
	NumObjectsInSceneLastFrame.clear();

	VoxelVolumes.clear();
	ObjectsInScene.clear();

	UpdateBLAS = false;

	ClearInstanceIDPool();
}

void VolumeRaytracer::Renderer::DX::VRDXScene::BuildTopLevelAccelerationStructures(VDXRenderer* renderer, const unsigned int& backBufferIndex)
{
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances;

	for (auto& elem : ObjectsInScene)
	{
		std::weak_ptr<Voxel::VVoxelVolume> volume = dynamic_cast<Scene::IVRenderableObject*>(elem->GetObjectDesc().LevelObject)->GetVoxelVolume();

		if (!volume.expired())
		{
			std::shared_ptr<VDXVoxelVolume> dxVolume = VoxelVolumes[volume.lock().get()];

			elem->ChangeGeometry(dxVolume->GetDesc().InstanceIndex, dxVolume->GetBLAS().AccelerationStructure->GetGPUVirtualAddress());
			elem->Update();

			instances.push_back(elem->GetInstanceDesc());
		}
	}

	if (TLAS.size() <= backBufferIndex || NumObjectsInSceneLastFrame.size() <= backBufferIndex || ObjectsInScene.size() != NumObjectsInSceneLastFrame[backBufferIndex])
	{
		if (TLAS.size() <= backBufferIndex)
		{
			TLAS.resize(backBufferIndex + 1, nullptr);
		}

		if (NumObjectsInSceneLastFrame.size() <= backBufferIndex)
		{
			NumObjectsInSceneLastFrame.resize(backBufferIndex + 1, 0);
		}

		if (TLAS[backBufferIndex] != nullptr)
		{
			delete TLAS[backBufferIndex];
			TLAS[backBufferIndex] = nullptr;
		}

		CPtr<ID3D12Device5> device = renderer->GetDXDevice();

		CPtr<ID3D12Resource> scratch;
		CPtr<ID3D12Resource> topLevelAS;
		CPtr<ID3D12Resource> instanceDescsResource;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
		topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		topLevelInputs.NumDescs = instances.size();

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
		device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

		CD3DX12_HEAP_PROPERTIES uavHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC uavResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		device->CreateCommittedResource(&uavHeapProps, D3D12_HEAP_FLAG_NONE, &uavResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&scratch));

		uavHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		uavResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		device->CreateCommittedResource(&uavHeapProps, D3D12_HEAP_FLAG_NONE, &uavResourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&topLevelAS));
		SetDXDebugName<ID3D12Resource>(topLevelAS, "Top Level AS");

		CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instances.size());

		device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&instanceDescsResource));

		SetDXDebugName<ID3D12Resource>(instanceDescsResource, "Instance Description resource");

		topLevelBuildDesc.DestAccelerationStructureData = topLevelAS->GetGPUVirtualAddress();
		topLevelInputs.InstanceDescs = instanceDescsResource->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();

		VDXAccelerationStructureBuffers* tlas = new VDXAccelerationStructureBuffers();
		tlas->Scratch = scratch;
		tlas->AccelerationStructureDesc = topLevelBuildDesc;
		tlas->AccelerationStructure = topLevelAS;
		tlas->InstanceDesc = instanceDescsResource;
		tlas->ResultDataMaxSizeInBytes = topLevelPrebuildInfo.ResultDataMaxSizeInBytes;

		TLAS[backBufferIndex] = tlas;
	}

	void* mappedData;
	TLAS[backBufferIndex]->InstanceDesc->Map(0, nullptr, &mappedData);
	memcpy(mappedData, instances.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instances.size());
	TLAS[backBufferIndex]->InstanceDesc->Unmap(0, nullptr);

	NumObjectsInSceneLastFrame[backBufferIndex] = ObjectsInScene.size();
}

void VolumeRaytracer::Renderer::DX::VRDXScene::AddVoxelVolume(std::weak_ptr<VDXRenderer> renderer, Voxel::VVoxelVolume* voxelVolume)
{
	if (VoxelVolumes.find(voxelVolume) == VoxelVolumes.end())
	{
		VDXVoxelVolumeDesc volumeDesc;

		if (GeometryInstancePool.pop(volumeDesc.InstanceIndex))
		{
			VRDXSceneObjectDescriptorHandles handles = ObjectResourcePool->GetObjectDescriptorHandles(volumeDesc.InstanceIndex);
			volumeDesc.Volume = voxelVolume;
			volumeDesc.GeometryCBHandle = handles.GeometryHandleCPU;
			volumeDesc.VolumeHandle = handles.VoxelVolumeHandleCPU;
			volumeDesc.TraversalHandle = handles.GeometryTraversalCPU;

			std::shared_ptr<VDXVoxelVolume> dxVolume = std::make_shared<VDXVoxelVolume>(renderer, volumeDesc);

			VoxelVolumes[voxelVolume] = dxVolume;

			UpdateBLAS = true;
		}
		else
		{
			V_LOG_ERROR("Maximum number of scene geometry reached!");
		}
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::RemoveVoxelVolume(Voxel::VVoxelVolume* voxelVolume)
{
	if (VoxelVolumes.find(voxelVolume) != VoxelVolumes.end())
	{
		GeometryInstancePool.push(VoxelVolumes[voxelVolume]->GetDesc().InstanceIndex);

		VoxelVolumes.erase(voxelVolume);

		UpdateBLAS = true;
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::AddLevelObject(std::weak_ptr<VDXRenderer> renderer, Scene::VLevelObject* levelObject)
{
	Scene::IVRenderableObject* renderableObject = dynamic_cast<Scene::IVRenderableObject*>(levelObject);

	if (renderableObject != nullptr)
	{
		if (!ContainsLevelObject(levelObject))
		{
			VDXLevelObjectDesc objectDesc;

			objectDesc.LevelObject = levelObject;

			std::shared_ptr<VDXLevelObject> dxLevelObj = std::make_shared<VDXLevelObject>(objectDesc);
			ObjectsInScene.push_back(dxLevelObj);
		}
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::RemoveLevelObject(Scene::VLevelObject* levelObject)
{
	auto& comparision = [levelObject](const std::shared_ptr<VDXLevelObject>& arrElement) {
		return arrElement->GetObjectDesc().LevelObject == levelObject;
	};

	if (levelObject != nullptr)
	{
		auto index = std::find_if(ObjectsInScene.begin(), ObjectsInScene.end(), comparision);

		if (index != ObjectsInScene.end())
		{
			ObjectsInScene.erase(index);
		}
	}
}	

void VolumeRaytracer::Renderer::DX::VRDXScene::ClearInstanceIDPool()
{
	size_t elem = 0;

	while(GeometryInstancePool.pop(elem)){}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::FillInstanceIDPool()
{
	if (!GeometryInstancePool.empty())
	{
		ClearInstanceIDPool();
	}

	for (size_t i = 0; i < MaxAllowedObjectData; i++)
	{
		GeometryInstancePool.push(i);
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::UpdateSceneConstantBuffer(std::weak_ptr<Scene::VScene> scene)
{
	VObjectPtr<Scene::VScene> scenePtr = scene.lock();

	VObjectPtr<Scene::VCamera> cam = scenePtr->GetActiveCamera();

	VVector lookAtVec = cam->Rotation.GetForwardVector();
	VVector upVec = cam->Rotation.GetUpVector();

	DirectX::XMVECTOR lookAtVecDX = DirectX::XMVectorSet(lookAtVec.X, lookAtVec.Y, lookAtVec.Z, 1.f);
	DirectX::XMVECTOR upVecDX = DirectX::XMVectorSet(upVec.X, upVec.Y, upVec.Z, 1.f);

	CameraPosition = DirectX::XMVectorSet(cam->Position.X, cam->Position.Y, cam->Position.Z, 1.f);

	ViewMatrix = DirectX::XMMatrixLookToRH(CameraPosition, lookAtVecDX, upVecDX);
	ProjectionMatrix = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(cam->FOVAngle), cam->AspectRatio, cam->NearClipPlane, cam->FarClipPlane);

	VVector dirLightDirection = scenePtr->GetActiveDirectionalLight()->Rotation.GetForwardVector();

	DirectionalLightDirection = DirectX::XMFLOAT3(dirLightDirection.X, dirLightDirection.Y, dirLightDirection.Z);
	DirectionalLightStrength = scenePtr->GetActiveDirectionalLight()->IlluminationStrength;
}

void VolumeRaytracer::Renderer::DX::VRDXScene::UpdateSceneGeometry(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VObjectPtr<Scene::VScene> scenePtr = scene.lock();

	boost::unordered_set<Voxel::VVoxelVolume*> volumesAdded =  scenePtr->GetVolumesAddedDuringFrame();
	boost::unordered_set<Voxel::VVoxelVolume*> volumesRemoved = scenePtr->GetVolumesRemovedDuringFrame();

	for (auto& elem : volumesRemoved)
	{
		RemoveVoxelVolume(elem);
		UpdateBLAS = true;
	}

	for (auto& elem : volumesAdded)
	{
		AddVoxelVolume(renderer, elem);
		UpdateBLAS = true;
	}

	for (auto& elem : VoxelVolumes)
	{
		if (elem.second->NeedsUpdate())
		{
			elem.second->UpdateFromVoxelVolume(renderer);
			UpdateBLAS = true;
		}
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::UpdateSceneObjects(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VObjectPtr<Scene::VScene> scenePtr = scene.lock();

	boost::unordered_set<Scene::VLevelObject*> objectsAdded = scenePtr->GetObjectsAddedDuringFrame();
	boost::unordered_set<Scene::VLevelObject*> objectsRemoved = scenePtr->GetObjectsRemovedDuringFrame();

	for (auto& elem : objectsRemoved)
	{
		RemoveLevelObject(elem);
	}

	for (auto& elem : objectsAdded)
	{
		AddLevelObject(renderer, elem);
	}
}

bool VolumeRaytracer::Renderer::DX::VRDXScene::ContainsLevelObject(Scene::VLevelObject* levelObject) const
{
	auto& comparision = [levelObject](const std::shared_ptr<VDXLevelObject>& arrElement){
		return arrElement->GetObjectDesc().LevelObject == levelObject;
	};

	return std::any_of(ObjectsInScene.begin(), ObjectsInScene.end(), comparision);
}

VolumeRaytracer::Renderer::DX::VRDXSceneObjectResourcePool::VRDXSceneObjectResourcePool(CPtr<ID3D12Device5> dxDevice, const size_t& maxObjects)
	:MaxObjects(maxObjects)
{
	VoxelVolumeHeap = new VDXDescriptorHeap(dxDevice, maxObjects, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	GeometryHeap = new VDXDescriptorHeap(dxDevice, maxObjects, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	GeometryTraversalHeap = new VDXDescriptorHeap(dxDevice, maxObjects, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
}

VolumeRaytracer::Renderer::DX::VRDXSceneObjectResourcePool::~VRDXSceneObjectResourcePool()
{
	if (VoxelVolumeHeap != nullptr)
	{
		delete VoxelVolumeHeap;
		VoxelVolumeHeap = nullptr;
	}

	if (GeometryHeap != nullptr)
	{
		delete GeometryHeap;
		GeometryHeap = nullptr;
	}

	if (GeometryTraversalHeap != nullptr)
	{
		delete GeometryTraversalHeap;
		GeometryTraversalHeap = nullptr;
	}
}

VolumeRaytracer::Renderer::DX::VRDXSceneObjectDescriptorHandles VolumeRaytracer::Renderer::DX::VRDXSceneObjectResourcePool::GetObjectDescriptorHandles(const size_t& objectIndex) const
{
	VRDXSceneObjectDescriptorHandles handles;

	handles.VoxelVolumeHandleCPU = VoxelVolumeHeap->GetCPUHandle(objectIndex);
	handles.VoxelVolumeHandleGPU = VoxelVolumeHeap->GetGPUHandle(objectIndex);
	handles.GeometryHandleCPU = GeometryHeap->GetCPUHandle(objectIndex);
	handles.GeometryHandleGPU = GeometryHeap->GetGPUHandle(objectIndex);
	handles.GeometryTraversalCPU = GeometryTraversalHeap->GetCPUHandle(objectIndex);
	handles.GeometryTraversalGPU = GeometryTraversalHeap->GetGPUHandle(objectIndex);

	return handles;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12DescriptorHeap> VolumeRaytracer::Renderer::DX::VRDXSceneObjectResourcePool::GetVoxelVolumeHeap() const
{
	return VoxelVolumeHeap->GetDescriptorHeap();
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12DescriptorHeap> VolumeRaytracer::Renderer::DX::VRDXSceneObjectResourcePool::GetGeometryHeap() const
{
	return GeometryHeap->GetDescriptorHeap();
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12DescriptorHeap> VolumeRaytracer::Renderer::DX::VRDXSceneObjectResourcePool::GetGeometryTraversalHeap() const
{
	return GeometryTraversalHeap->GetDescriptorHeap();
}

size_t VolumeRaytracer::Renderer::DX::VRDXSceneObjectResourcePool::GetMaxObjectsAllowed() const
{
	return MaxObjects;
}
