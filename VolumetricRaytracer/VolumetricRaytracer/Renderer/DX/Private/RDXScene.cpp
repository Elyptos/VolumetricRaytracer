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
#include "DXTexture2D.h"
#include "RDXVoxelVolume.h"
#include "RDXLevelObject.h"
#include "RenderableObject.h"
#include "Logger.h"
#include "../../Scene/Public/PointLight.h"
#include "../../Scene/Public/SpotLight.h"
#include "DXLightFactory.h"
#include "../../Core/Public/MathHelpers.h"
#include "Material.h"
#include "VoxelVolume.h"

void VolumeRaytracer::Renderer::DX::VRDXScene::InitFromScene(std::weak_ptr<VRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VRScene::InitFromScene(renderer, scene);
	EnvironmentMap = std::static_pointer_cast<VDXTextureCube>(scene.lock()->GetEnvironmentTexture());

	VObjectPtr<VDXRenderer> dxRenderer = std::static_pointer_cast<VDXRenderer>(renderer.lock());
	BuildStaticResources(dxRenderer);

	InitSceneGeometry(dxRenderer, scene);
	InitSceneObjects(dxRenderer, scene);
}

void VolumeRaytracer::Renderer::DX::VRDXScene::Cleanup()
{
	VRScene::Cleanup();
	CleanupStaticResources();
}

void VolumeRaytracer::Renderer::DX::VRDXScene::BuildStaticResources(VObjectPtr<VDXRenderer> renderer)
{
	CleanupStaticResources();

	FillInstanceIDPool();
	FillTextureInstanceIDPool();

	if (DXSceneDescriptorHeap == nullptr)
	{
		DXSceneDescriptorHeap = new VDXDescriptorHeap(renderer->GetDXDevice(), VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT + MaxAllowedObjectData * 3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}

	if (DXSceneDescriptorHeapSamplers == nullptr)
	{
		DXSceneDescriptorHeapSamplers = new VDXDescriptorHeap(renderer->GetDXDevice(), VDXConstants::STATIC_SCENERY_SAMPLER_COUNT, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}

	if (DXSceneLightsDescriptorHeap == nullptr)
	{
		DXSceneLightsDescriptorHeap = new VDXDescriptorHeap(renderer->GetDXDevice(), VDXConstants::BACK_BUFFER_COUNT * (MaxAllowedPointLights + MaxAllowedSpotLights), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	}

	if (ObjectResourcePool == nullptr)
	{
		ObjectResourcePool = new VRDXSceneObjectResourcePool(renderer->GetDXDevice(), MaxAllowedObjectData);
	}

	AllocateDefaultTextures(renderer);
	AllocSceneConstantBuffer(renderer.get());
	AllocLightConstantBuffers(renderer.get());
	InitEnvironmentMap(renderer.get());
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
	constantBufferData.numPointLights = VMathHelpers::Min((UINT)PointLights.size(), MaxAllowedPointLights);
	constantBufferData.numSpotLights = VMathHelpers::Min((UINT)SpotLights.size(), MaxAllowedSpotLights);

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
	UpdateLights(backBufferIndex);

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

D3D12_CPU_DESCRIPTOR_HANDLE VolumeRaytracer::Renderer::DX::VRDXScene::GetSceneLightsHeapStart(const unsigned int& backBufferIndex) const
{
	return DXSceneLightsDescriptorHeap->GetCPUHandle(backBufferIndex * (MaxAllowedPointLights + MaxAllowedSpotLights));
}

void VolumeRaytracer::Renderer::DX::VRDXScene::InitEnvironmentMap(VDXRenderer* renderer)
{
	if (EnvironmentMap != nullptr)
	{
		renderer->CreateSRVDescriptor(EnvironmentMap, DXSceneDescriptorHeap->GetCPUHandle(0));
		renderer->UploadToGPU(EnvironmentMap);

		D3D12_SAMPLER_DESC samplerDesc = {};

		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;

		renderer->GetDXDevice()->CreateSampler(&samplerDesc, DXSceneDescriptorHeapSamplers->GetCPUHandle(0));
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
		std::shared_ptr<Scene::VLevelObject> levelObject = elem.lock();
		std::shared_ptr<Scene::VPointLight> pointLight = std::dynamic_pointer_cast<Scene::VPointLight>(levelObject);

		if (pointLight != nullptr)
		{
			AddPointLight(pointLight.get());
			continue;
		}

		std::shared_ptr<Scene::VSpotLight> spotLight = std::dynamic_pointer_cast<Scene::VSpotLight>(levelObject);
		if (spotLight != nullptr)
		{
			AddSpotLight(spotLight.get());
			continue;
		}

		AddLevelObject(renderer, elem.lock().get());
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::AllocateDefaultTextures(std::weak_ptr<VDXRenderer> renderer)
{
	DefaultAlbedoTexture = std::dynamic_pointer_cast<VDXTexture2D>(Renderer::VTextureFactory::CreateTexture2D(renderer, 1, 1, 1));
	DefaultAlbedoTexture->SetPixel(0, 0, 0, VColor::WHITE);

	DefaultNormalTexture = std::dynamic_pointer_cast<VDXTexture2D>(Renderer::VTextureFactory::CreateTexture2D(renderer, 1, 1, 1));
	DefaultNormalTexture->SetPixel(0, 0, 0, VColor(0.5f, 0.5f, 1.f, 1.f));

	DefaultRMTexture = std::dynamic_pointer_cast<VDXTexture2D>(Renderer::VTextureFactory::CreateTexture2D(renderer, 1, 1, 1));
	DefaultRMTexture->SetPixel(0, 0, 0, VColor(1.f, 1.f, 0.f, 1.f));

	std::shared_ptr<VDXRenderer> rendererPtr = renderer.lock();

	rendererPtr->CreateSRVDescriptor(DefaultAlbedoTexture, DXSceneDescriptorHeap->GetCPUHandle(1));
	rendererPtr->CreateSRVDescriptor(DefaultNormalTexture, DXSceneDescriptorHeap->GetCPUHandle(2));
	rendererPtr->CreateSRVDescriptor(DefaultRMTexture, DXSceneDescriptorHeap->GetCPUHandle(3));

	rendererPtr->UploadToGPU(DefaultAlbedoTexture);
	rendererPtr->UploadToGPU(DefaultNormalTexture);
	rendererPtr->UploadToGPU(DefaultRMTexture);

	D3D12_SAMPLER_DESC samplerDesc = {};

	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;

	rendererPtr->GetDXDevice()->CreateSampler(&samplerDesc, DXSceneDescriptorHeapSamplers->GetCPUHandle(1));
}

void VolumeRaytracer::Renderer::DX::VRDXScene::AllocLightConstantBuffers(VDXRenderer* renderer)
{
	CPtr<ID3D12Device5> dxDevice = renderer->GetDXDevice();
	
	size_t pointLightBufferSize = VDXHelper::Align(sizeof(VPointLightBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	size_t spotLightBufferSize = VDXHelper::Align(sizeof(VSpotLightBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	ScenePointLightBuffers.resize(VDXConstants::BACK_BUFFER_COUNT);
	SceneSpotLightBuffers.resize(VDXConstants::BACK_BUFFER_COUNT);

	for (auto& elem : ScenePointLightBuffers)
	{
		elem.resize(MaxAllowedPointLights);

		for (auto& pointLightBuffer : elem)
		{
			pointLightBuffer = std::make_shared<VD3DConstantBuffer>();

			CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(pointLightBufferSize, D3D12_RESOURCE_FLAG_NONE);
			CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

			dxDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pointLightBuffer->Resource));

			uint8_t* dataPtr = nullptr;

			CD3DX12_RANGE mapRange(0, 0);
			pointLightBuffer->Resource->Map(0, &mapRange, reinterpret_cast<void**>(&dataPtr));
			pointLightBuffer->DataPtr = dataPtr;
			pointLightBuffer->BufferSize = pointLightBufferSize;
		}
	}

	for (auto& elem : SceneSpotLightBuffers)
	{
		elem.resize(MaxAllowedSpotLights);

		for (auto& spotLightBuffer : elem)
		{
			spotLightBuffer = std::make_shared<VD3DConstantBuffer>();

			CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(spotLightBufferSize, D3D12_RESOURCE_FLAG_NONE);
			CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

			dxDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&spotLightBuffer->Resource));

			uint8_t* dataPtr = nullptr;

			CD3DX12_RANGE mapRange(0, 0);
			spotLightBuffer->Resource->Map(0, &mapRange, reinterpret_cast<void**>(&dataPtr));
			spotLightBuffer->DataPtr = dataPtr;
			spotLightBuffer->BufferSize = spotLightBufferSize;
		}
	}

	for (int frame = 0; frame < VDXConstants::BACK_BUFFER_COUNT; frame++)
	{
		for (int pLIndex = 0; pLIndex < MaxAllowedPointLights; pLIndex++)
		{
			std::shared_ptr<VD3DConstantBuffer> buffer = ScenePointLightBuffers[frame][pLIndex];

			UINT descIndex = 0;

			DXSceneLightsDescriptorHeap->AllocateDescriptor(&buffer->CPUDescHandle, &buffer->GPUDescHandle, descIndex);

			renderer->CreateCBDescriptor(buffer->Resource, buffer->BufferSize, buffer->CPUDescHandle);
		}

		for (int sLIndex = 0; sLIndex < MaxAllowedSpotLights; sLIndex++)
		{
			std::shared_ptr<VD3DConstantBuffer> buffer = SceneSpotLightBuffers[frame][sLIndex];

			UINT descIndex = 0;

			DXSceneLightsDescriptorHeap->AllocateDescriptor(&buffer->CPUDescHandle, &buffer->GPUDescHandle, descIndex);

			renderer->CreateCBDescriptor(buffer->Resource, buffer->BufferSize, buffer->CPUDescHandle);
		}
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

	if (DXSceneLightsDescriptorHeap != nullptr)
	{
		delete DXSceneLightsDescriptorHeap;
		DXSceneLightsDescriptorHeap = nullptr;
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

	GeometryTextures.clear();

	SceneConstantBuffers.clear();
	SceneConstantBufferDataPtrs.clear();

	ScenePointLightBuffers.clear();
	SceneSpotLightBuffers.clear();

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
	PointLights.clear();
	SpotLights.clear();

	UpdateBLAS = false;

	DefaultAlbedoTexture = nullptr;
	DefaultNormalTexture = nullptr;
	DefaultRMTexture = nullptr;

	ClearInstanceIDPool();
	ClearTextureInstanceIDPool();
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

void VolumeRaytracer::Renderer::DX::VRDXScene::AddPointLight(Scene::VPointLight* pointLight)
{
	if (pointLight != nullptr && std::find(PointLights.begin(), PointLights.end(), pointLight) == PointLights.end())
	{
		PointLights.push_back(pointLight);
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::RemovePointLight(Scene::VPointLight* pointLight)
{
	if (pointLight != nullptr)
	{
		auto index = std::find(PointLights.begin(), PointLights.end(), pointLight);

		if (index != PointLights.end())
		{
			PointLights.erase(index);
		}
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::AddSpotLight(Scene::VSpotLight* spotLight)
{
	if (spotLight != nullptr && std::find(SpotLights.begin(), SpotLights.end(), spotLight) == SpotLights.end())
	{
		SpotLights.push_back(spotLight);
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::RemoveSpotLight(Scene::VSpotLight* spotLight)
{
	if (spotLight != nullptr)
	{
		auto index = std::find(SpotLights.begin(),SpotLights.end(), spotLight);

		if (index != SpotLights.end())
		{
			SpotLights.erase(index);
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

void VolumeRaytracer::Renderer::DX::VRDXScene::ClearTextureInstanceIDPool()
{
	size_t elem = 0;

	while (GeometryTextureInstancePool.pop(elem)) {}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::FillTextureInstanceIDPool()
{
	if (!GeometryTextureInstancePool.empty())
	{
		ClearTextureInstanceIDPool();
	}

	for (size_t i = VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT; i < (MaxAllowedObjectData * 3 + VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT); i++)
	{
		GeometryTextureInstancePool.push(i);
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

void VolumeRaytracer::Renderer::DX::VRDXScene::UpdateLights(const unsigned int& backBufferIndex)
{
	for (int i = 0; i < PointLights.size(); i++)
	{
		if (i < MaxAllowedPointLights)
		{
			VPointLightBuffer lightBuffer = VDXLightFactory::GetLightBuffer(PointLights[i]);

			memcpy(ScenePointLightBuffers[backBufferIndex][i]->DataPtr, &lightBuffer, sizeof(VPointLightBuffer));
		}
		else
		{
			break;
		}
	}

	for (int i = 0; i < SpotLights.size(); i++)
	{
		if (i < MaxAllowedSpotLights)
		{
			VSpotLightBuffer lightBuffer = VDXLightFactory::GetLightBuffer(SpotLights[i]);

			memcpy(SceneSpotLightBuffers[backBufferIndex][i]->DataPtr, &lightBuffer, sizeof(VSpotLightBuffer));
		}
		else
		{
			break;
		}
	}
}

void VolumeRaytracer::Renderer::DX::VRDXScene::UpdateSceneGeometry(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VObjectPtr<Scene::VScene> scenePtr = scene.lock();

	boost::unordered_set<Voxel::VVoxelVolume*> volumesAdded =  scenePtr->GetVolumesAddedDuringFrame();
	boost::unordered_set<Voxel::VVoxelVolume*> volumesRemoved = scenePtr->GetVolumesRemovedDuringFrame();

	SyncGeometryTextures(renderer, scene);

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
			VDXVoxelVolumeTextureIndices textureIndices;

			VMaterial material = elem.first->GetMaterial();

			if (GeometryTextures.find(material.AlbedoTexturePath) != GeometryTextures.end())
			{
				textureIndices.AlbedoIndex = GeometryTextures[material.AlbedoTexturePath].DescriptorIndex - VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT + 3;
			}
			else
			{
				textureIndices.AlbedoIndex = 0;
			}

			if (GeometryTextures.find(material.NormalTexturePath) != GeometryTextures.end())
			{
				textureIndices.NormalIndex = GeometryTextures[material.NormalTexturePath].DescriptorIndex - VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT + 3;
			}
			else
			{
				textureIndices.NormalIndex = 1;
			}

			if (GeometryTextures.find(material.RMTexturePath) != GeometryTextures.end())
			{
				textureIndices.RMIndex = GeometryTextures[material.RMTexturePath].DescriptorIndex - VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT + 3;
			}
			else
			{
				textureIndices.RMIndex = 2;
			}

			elem.second->SetTextures(textureIndices);
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
		Scene::VPointLight* pointLight = dynamic_cast<Scene::VPointLight*>(elem);

		if (pointLight != nullptr)
		{
			RemovePointLight(pointLight);
			continue;
		}

		Scene::VSpotLight* spotLight = dynamic_cast<Scene::VSpotLight*>(elem);
		if (spotLight != nullptr)
		{
			RemoveSpotLight(spotLight);
			continue;
		}

		RemoveLevelObject(elem);
	}

	for (auto& elem : objectsAdded)
	{
		Scene::VPointLight* pointLight = dynamic_cast<Scene::VPointLight*>(elem);

		if (pointLight != nullptr)
		{
			AddPointLight(pointLight);
			continue;
		}

		Scene::VSpotLight* spotLight = dynamic_cast<Scene::VSpotLight*>(elem);
		if (spotLight != nullptr)
		{
			AddSpotLight(spotLight);
			continue;
		}

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

void VolumeRaytracer::Renderer::DX::VRDXScene::SyncGeometryTextures(std::weak_ptr<VDXRenderer> renderer, std::weak_ptr<Scene::VScene> scene)
{
	VObjectPtr<Scene::VScene> scenePtr = scene.lock();

	boost::unordered_set<std::wstring> usedTextures = scenePtr->GetAllReferencedGeometryTextures();

	std::vector<std::wstring> texturesToRemove;
	
	for (const auto& elem : GeometryTextures)
	{
		if (usedTextures.find(elem.first) == usedTextures.end())
		{
			texturesToRemove.push_back(elem.first);
		}
	}

	for (const auto& elem : texturesToRemove)
	{
		if (GeometryTextures[elem].Texture != nullptr)
		{
			GeometryTextureInstancePool.push(GeometryTextures[elem].DescriptorIndex);
			GeometryTextures.erase(elem);
		}
	}

	for (const auto& elem : usedTextures)
	{
		if (GeometryTextures.find(elem) == GeometryTextures.end())
		{
			VDRXGeometryTextureReference textureRef;
			textureRef.DescriptorIndex = 0;
			textureRef.Texture = nullptr;

			if (!GeometryTextureInstancePool.empty())
			{
				textureRef.Texture = std::dynamic_pointer_cast<VDXTexture2D>(Renderer::VTextureFactory::LoadTexture2DFromFile(renderer, elem));

				if (textureRef.Texture != nullptr)
				{
					GeometryTextureInstancePool.pop(textureRef.DescriptorIndex);

					renderer.lock()->CreateSRVDescriptor(textureRef.Texture, DXSceneDescriptorHeap->GetCPUHandle(textureRef.DescriptorIndex));
					renderer.lock()->UploadToGPU(textureRef.Texture);
				}

				GeometryTextures[elem] = textureRef;
			}
		}
	}
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
