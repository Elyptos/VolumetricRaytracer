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

#include "DXRenderer.h"
#include "Logger.h"
#include "DXConstants.h"
#include "RaytracingHlsl.h"
#include "RDXScene.h"
#include "DXDescriptorHeap.h"
#include "Camera.h"
#include "DXTextureCube.h"
#include "Compiled/Raytracing.hlsl.h"
#include "Compiled/Raytracing_Unlit.hlsl.h"
#include "Compiled/Raytracing_NoTex.hlsl.h"
#include "Compiled/Raytracing_NoTex_Unlit.hlsl.h"
#include "Compiled/Raytracing_Cube.hlsl.h"
#include "Compiled/Raytracing_Cube_Unlit.hlsl.h"
#include "Compiled/Raytracing_Cube_NoTex.hlsl.h"
#include "Compiled/Raytracing_Cube_NoTex_Unlit.hlsl.h"
#include <string>
#include <sstream>
#include <vector>

#undef _DEBUG

void VolumeRaytracer::Renderer::DX::VDXRenderer::Render()
{
	if (IsActive())
	{
		if (WindowRenderTarget != nullptr && SceneToRender != nullptr && !SceneRef.expired())
		{
			std::weak_ptr<VDXRenderer> weakThis = std::static_pointer_cast<VDXRenderer>(shared_from_this());

			UploadPendingTexturesToGPU();

			SceneRef.lock()->GetActiveCamera()->AspectRatio = (float)WindowRenderTarget->GetWidth() / (float)WindowRenderTarget->GetHeight();
			SceneToRender->SyncWithScene(weakThis, SceneRef);
			SceneToRender->PrepareForRendering(weakThis, WindowRenderTarget->GetCurrentBufferIndex());

			PrepareForRendering();
			DoRendering();
			CopyRaytracingOutputToBackbuffer();

			ExecuteCommandList();

			WindowRenderTarget->Present(0, DXGI_PRESENT_ALLOW_TEARING);

			MoveToNextFrame();
		}
	}
	else
	{
		V_LOG_WARNING("Calling Render on uninitialized renderer!");
	}
}


void VolumeRaytracer::Renderer::DX::VDXRenderer::Start()
{
	SetupRenderer();
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::Stop()
{
	DestroyRenderer();
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::SetWindowHandle(HWND hwnd, unsigned int width, unsigned int height)
{
	ClearWindowHandle();

	WindowRenderTarget = new VDXWindowRenderTargetHandler(DXGIFactory.Get(), Device.Get(), RenderCommandQueue.Get(), hwnd, width, height);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::ClearWindowHandle()
{
	WaitForGPU();
	ReleaseWindowResources();
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::ResizeRenderOutput(unsigned int width, unsigned int height)
{
	if (HasValidWindow())
	{
		WaitForGPU();

		WindowRenderTarget->Resize(Device.Get(), width, height);
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::BuildBottomLevelAccelerationStructure(std::vector<VDXAccelerationStructureBuffers> bottomLevelAS)
{
	ID3D12GraphicsCommandList5* commandList = ComputeCommandHandler->StartCommandRecording();

	for (auto& as : bottomLevelAS)
	{
		commandList->BuildRaytracingAccelerationStructure(&as.AccelerationStructureDesc, 0, nullptr);
	}

	ComputeCommandHandler->ExecuteCommandQueue();
	ComputeCommandHandler->WaitForGPU();
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::SetSceneToRender(VObjectPtr<Scene::VScene> scene)
{
	VRenderer::SetSceneToRender(scene);

	std::weak_ptr<VDXRenderer> weakThis = std::static_pointer_cast<VDXRenderer>(shared_from_this());

	DeleteScene();

	SceneToRender = new VRDXScene();
	SceneToRender->InitFromScene(weakThis, scene);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitializeTexture(VObjectPtr<VTexture> texture)
{
	std::weak_ptr<VDXRenderer> weakThis = std::static_pointer_cast<VDXRenderer>(shared_from_this());

	IDXRenderableTexture* renderableTexture = dynamic_cast<IDXRenderableTexture*>(texture.get());

	if (renderableTexture != nullptr)
	{
		if (renderableTexture->GetOwnerRenderer().expired())
		{
			renderableTexture->SetOwnerRenderer(weakThis);
			renderableTexture->InitGPUResource(this);
		}
		else
		{
			V_LOG_ERROR("Texture initialization failed because texture belongs to another renderer!");
		}
	}
	else
	{
		V_LOG_ERROR("Textures initialization failed because texture is not of type IDXRenderableTexture!");
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::CreateSRVDescriptor(VObjectPtr<VTexture> texture, const D3D12_CPU_DESCRIPTOR_HANDLE& descHandle)
{
	IDXRenderableTexture* renderableTexture = dynamic_cast<IDXRenderableTexture*>(texture.get());

	if (renderableTexture != nullptr)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC resourceViewDesc = {};

		resourceViewDesc.Format = renderableTexture->GetDXGIFormat();
		resourceViewDesc.ViewDimension = renderableTexture->GetSRVDimension();
		resourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		resourceViewDesc.TextureCube.MostDetailedMip = 0;
		resourceViewDesc.TextureCube.MipLevels = texture->GetMipCount();
		resourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		Device->CreateShaderResourceView(renderableTexture->GetDXGPUResource().Get(), &resourceViewDesc, descHandle);
	}
	else
	{
		V_LOG_ERROR("SRV creation failed because texture is not of type IDXRenderableTexture!");
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::CreateCBDescriptor(CPtr<ID3D12Resource> resource, const size_t& resourceSize, const D3D12_CPU_DESCRIPTOR_HANDLE& descHandle)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbViewDesc = {};

	cbViewDesc.BufferLocation = resource->GetGPUVirtualAddress();
	cbViewDesc.SizeInBytes = resourceSize;

	Device->CreateConstantBufferView(&cbViewDesc, descHandle);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::UploadToGPU(VObjectPtr<VTexture> texture)
{
	std::shared_ptr<IDXRenderableTexture> renderableTexture = std::dynamic_pointer_cast<IDXRenderableTexture>(texture);

	if (renderableTexture != nullptr && TexturesToUpload.find(renderableTexture.get()) == TexturesToUpload.end())
	{
		VDXTextureUploadPayload uploadPayload = renderableTexture->BeginGPUUpload();
		DXTextureUploadInfo uploadInfo;
		uploadInfo.Texture = renderableTexture;
		uploadInfo.UploadPayload = uploadPayload;

		TexturesToUpload[renderableTexture.get()] = uploadInfo;
	}
}

bool VolumeRaytracer::Renderer::DX::VDXRenderer::HasValidWindow() const
{
	return WindowRenderTarget != nullptr;
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::SetupRenderer()
{
	if (!IsActive())
	{

		V_LOG("Starting DirectX 12 renderer");

		UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
		V_LOG("Enabling DirectX debug layer");
		SetupDebugLayer();

		dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&DXGIFactory))))
		{
			V_LOG_FATAL("DXGI initialization failed!");
			ReleaseInternalVariables();
			return;
		}

		CPtr<IDXGIAdapter3> adapter = SelectGPU();

		if (adapter == nullptr)
		{
			V_LOG_FATAL("No suitable DirectX device found on this system!");
			ReleaseInternalVariables();
			return;
		}

		if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device))))
		{
			V_LOG_FATAL("DirectX device creation failed!");
			ReleaseInternalVariables();
			return;
		}

#ifdef _DEBUG
		SetupDebugQueue();
#endif

		D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
		ZeroMemory(&commandQueueDesc, sizeof(commandQueueDesc));

		commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		commandQueueDesc.NodeMask = 0;

		if (FAILED(Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&RenderCommandQueue))))
		{
			V_LOG_FATAL("Command queue creation failed!");
			ReleaseInternalVariables();
			return;
		}

		SetDXDebugName<ID3D12CommandQueue>(RenderCommandQueue, "VR Command Queue");

		for (UINT i = 0; i < VDXConstants::BACK_BUFFER_COUNT; i++)
		{
			WindowCommandAllocators.push_back(nullptr);

			if (FAILED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&WindowCommandAllocators[i]))))
			{
				V_LOG_FATAL("Command allocator creation failed!");
				ReleaseInternalVariables();
				return;
			}

			SetDXDebugName<ID3D12CommandAllocator>(WindowCommandAllocators[i], "VR Command Allocator " + i);
		}

		if (FAILED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, WindowCommandAllocators[0].Get(), NULL, IID_PPV_ARGS(&CommandList))))
		{
			V_LOG_FATAL("Command list creation failed!");
			ReleaseInternalVariables();
			return;
		}

		SetDXDebugName<ID3D12CommandList>(CommandList, "VR Command List");

		CommandList->Close();

		UploadCommandHandler = new VDXGPUCommand(Device.Get(), D3D12_COMMAND_LIST_TYPE_COPY, "UploadCommandHandler");
		ComputeCommandHandler = new VDXGPUCommand(Device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE, "ComputeCommandHandler");

		InitRendererDescriptorHeap();
		InitializeGlobalRootSignature();
		InitRaytracingPipeline();
		CreateShaderTables();

		if (FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence))))
		{
			V_LOG_FATAL("DX Fence creation failed!");
			ReleaseInternalVariables();
			return;
		}

		SetDXDebugName<ID3D12Fence>(Fence, "Volume Raytracer Fence");

		FenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);

		IsInitialized = true;

		V_LOG("DirectX 12 renderer initialized!");
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::DestroyRenderer()
{
	if (IsActive())
	{
		V_LOG("Shutting down DirectX 12 renderer");

		ReleaseInternalVariables();

		IsInitialized = false;

		V_LOG("DirectX 12 renderer stopped");
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::SetupDebugLayer()
{
	CPtr<ID3D12Debug> debugController;
	CPtr<ID3D12Debug1> debugControllerExtended;

	if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		V_LOG_ERROR("Unable to retrieve debug interface for DirectX 12!");
		return;
	}

	debugController->QueryInterface(IID_PPV_ARGS(&debugControllerExtended));

	debugControllerExtended->EnableDebugLayer();
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::SetupDebugQueue()
{
	CPtr<ID3D12InfoQueue> infoQueue;

	if (SUCCEEDED(Device.As(&infoQueue)))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

		std::vector<D3D12_MESSAGE_SEVERITY> severities =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		std::vector<D3D12_MESSAGE_ID> denyIds =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER filter = {};

		filter.DenyList.NumSeverities = (UINT)severities.size();
		filter.DenyList.pSeverityList = severities.data();
		filter.DenyList.NumIDs = (UINT)denyIds.size();
		filter.DenyList.pIDList = denyIds.data();

		infoQueue->PushStorageFilter(&filter);
	}
}

VolumeRaytracer::Renderer::DX::CPtr<IDXGIAdapter3> VolumeRaytracer::Renderer::DX::VDXRenderer::SelectGPU()
{
	V_LOG("Enumerating DirectX enabled devices on this system");

	CPtr<IDXGIAdapter1> adapter1 = nullptr;
	CPtr<IDXGIAdapter3> adapter3 = nullptr;

	size_t maxDedicatedVideoMemory = 0;

	for (UINT i = 0; DXGIFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; i++)
	{
		DXGI_ADAPTER_DESC1 adapterDesc = {};
		adapter1->GetDesc1(&adapterDesc);

		std::ostringstream sstream;

		std::wstring adapterName(adapterDesc.Description);

		sstream << "Found device: " << std::string(adapterName.begin(), adapterName.end());

		V_LOG(sstream.str());

		if (adapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			CPtr<ID3D12Device5> testDevice;

			if (SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&testDevice))))
			{
				D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureOptions = {};

				if (SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureOptions, sizeof(featureOptions))))
				{
					if (featureOptions.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
					{
						V_LOG("Device test finished! Using selected device!");

						testDevice.Reset();

						adapter1.As(&adapter3);

						return adapter3;
					}
					else
					{
						V_LOG_WARNING("Device does not support raytracing! Skipping");
					}
				}

				testDevice.Reset();
			}
			else
			{
				V_LOG_WARNING("Failed initializing test for selected device! Skipping");
			}
		}
		else
		{
			V_LOG_WARNING("Supported video memory of device is not enough! Skipping");
		}
	}

	return adapter3;
}

uint64_t VolumeRaytracer::Renderer::DX::VDXRenderer::SignalFence(CPtr<ID3D12CommandQueue> commandQueue, CPtr<ID3D12Fence> fence, uint64_t& fenceValue)
{
	uint64_t fenceValueForSignal = ++fenceValue;

	if (FAILED(commandQueue->Signal(fence.Get(), fenceValueForSignal)))
	{
		V_LOG_WARNING("Unable to signal DX fence!");
	}

	return fenceValueForSignal;
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::WaitForFenceValue(CPtr<ID3D12Fence> fence, const uint64_t fenceValue, HANDLE fenceEvent)
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::ReleaseWindowResources()
{
	if (WindowRenderTarget != nullptr)
	{
		WindowRenderTarget = nullptr;
		delete WindowRenderTarget;
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::ReleaseInternalVariables()
{
	if (UploadCommandHandler != nullptr)
	{
		delete UploadCommandHandler;
		UploadCommandHandler = nullptr;
	}

	if (ComputeCommandHandler != nullptr)
	{
		delete ComputeCommandHandler;
		ComputeCommandHandler = nullptr;
	}

	DeleteScene();

	if (RendererSamplerDescriptorHeap != nullptr)
	{
		delete RendererSamplerDescriptorHeap;
		RendererSamplerDescriptorHeap = nullptr;
	}

	if (RendererDescriptorHeap != nullptr)
	{
		delete RendererDescriptorHeap;
		RendererDescriptorHeap = nullptr;
	}

	for(auto& elem : ShaderTableMiss)
	{
		if (elem != nullptr)
		{
			elem.Reset();
			elem = nullptr;
		}
	}

	ShaderTableMiss.clear();

	for (auto& elem : ShaderTableHitGroups)
	{
		if (elem != nullptr)
		{
			elem.Reset();
			elem = nullptr;
		}
	}

	ShaderTableHitGroups.clear();

	for (auto& elem : ShaderTableRayGen)
	{
		if (elem != nullptr)
		{
			elem.Reset();
			elem = nullptr;
		}
	}

	ShaderTableRayGen.clear();
	DXRStateObjects.clear();

	if (Fence != nullptr)
	{
		CloseHandle(FenceEvent);

		Fence.Reset();
		Fence = nullptr;
	}

	ReleaseWindowResources();

	if (PipelineState != nullptr)
	{
		PipelineState.Reset();
		PipelineState = nullptr;
	}

	if (CommandList != nullptr)
	{
		CommandList.Reset();
		CommandList = nullptr;
	}

	for (auto& commandAllocator : WindowCommandAllocators)
	{
		commandAllocator.Reset();
		commandAllocator = nullptr;
	}

	WindowCommandAllocators.clear();

	if (RenderCommandQueue != nullptr)
	{
		RenderCommandQueue.Reset();
		RenderCommandQueue = nullptr;
	}

	if (GlobalRootSignature != nullptr)
	{
		GlobalRootSignature.Reset();
		GlobalRootSignature = nullptr;
	}

	if (DXGIFactory != nullptr)
	{
		DXGIFactory.Reset();
		DXGIFactory = nullptr;
	}

	if (Device != nullptr)
	{
#ifdef _DEBUG
		V_LOG("Gathering possible live objects");

		CPtr<ID3D12DebugDevice> debugDevice;
		Device->QueryInterface(IID_PPV_ARGS(&debugDevice));

		debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
#endif

		Device.Reset();
		Device = nullptr;
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitRendererDescriptorHeap()
{
	if (RendererDescriptorHeap == nullptr)
	{
		RendererDescriptorHeap = new VDXDescriptorHeapRingBuffer(GetDXDevice(), VDXConstants::SRV_CV_UAV_HEAP_SIZE_PER_FRAME * VDXConstants::BACK_BUFFER_COUNT);
		RendererSamplerDescriptorHeap = new VDXDescriptorHeapRingBuffer(GetDXDevice(), VDXConstants::SAMPLER_HEAP_SIZE_PER_FRAME * VDXConstants::BACK_BUFFER_COUNT, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitializeGlobalRootSignature()
{
	/// <summary>
	/// SRV
	/// 0 = Acceleration Structure
	///	1 = Environment Map
	/// 2 - n = Voxel Volumes
	/// 
	/// Sampler
	/// 0 = Environment Map Sampler
	/// 
	/// CB
	/// 0 = Scene Constant Buffer
	/// 1 - n = Geometry Constant Buffer
	/// 
	/// UAV
	/// 0 = Output Texture
	/// </summary>

	CD3DX12_DESCRIPTOR_RANGE outputViewDescRange = {};
	outputViewDescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	size_t scenerySRVCount = VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT + MaxAllowedObjectData * 3;

	CD3DX12_DESCRIPTOR_RANGE sceneDescTable[4];
	sceneDescTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, scenerySRVCount, 1);
	sceneDescTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, VDXConstants::STATIC_SCENERY_SAMPLER_COUNT, 0);
	sceneDescTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, VolumeRaytracer::MaxAllowedPointLights, 1);
	sceneDescTable[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, VolumeRaytracer::MaxAllowedSpotLights, 1 + MaxAllowedPointLights);

	CD3DX12_DESCRIPTOR_RANGE geometryDescTable[3];
	geometryDescTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, VolumeRaytracer::MaxAllowedObjectData, scenerySRVCount + 1);
	geometryDescTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, VolumeRaytracer::MaxAllowedObjectData, 1 + MaxAllowedPointLights + MaxAllowedSpotLights);
	geometryDescTable[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, VolumeRaytracer::MaxAllowedObjectData, scenerySRVCount + 1 + VolumeRaytracer::MaxAllowedObjectData);

	CD3DX12_ROOT_PARAMETER rootParameters[EGlobalRootSignature::Max];
	rootParameters[EGlobalRootSignature::OutputView].InitAsDescriptorTable(1, &outputViewDescRange);
	rootParameters[EGlobalRootSignature::AccelerationStructure].InitAsShaderResourceView(0);
	rootParameters[EGlobalRootSignature::SceneConstant].InitAsConstantBufferView(0);
	rootParameters[EGlobalRootSignature::SceneTextures].InitAsDescriptorTable(1, &sceneDescTable[0]);
	rootParameters[EGlobalRootSignature::SceneSamplers].InitAsDescriptorTable(1, &sceneDescTable[1]);
	rootParameters[EGlobalRootSignature::SceneLights].InitAsDescriptorTable(2, &sceneDescTable[2]);
	rootParameters[EGlobalRootSignature::GeometryVolumes].InitAsDescriptorTable(1, &geometryDescTable[0]);
	rootParameters[EGlobalRootSignature::GeometryConstants].InitAsDescriptorTable(1, &geometryDescTable[1]);
	rootParameters[EGlobalRootSignature::GeometryTraversal].InitAsDescriptorTable(1, &geometryDescTable[2]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(ARRAYSIZE(rootParameters), rootParameters);

	CPtr<ID3DBlob> blob;
	CPtr<ID3DBlob> error;

	if (FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error)))
	{
		V_LOG_ERROR("Failed to create global root signature!");

		std::wstring wString(static_cast<wchar_t*>(error->GetBufferPointer()));
		std::string reason = std::string(wString.begin(), wString.end());

		V_LOG_ERROR(reason);
	}
	else
	{
		Device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&GlobalRootSignature));
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitRaytracingPipeline()
{
	DXRStateObjects.clear();
	DXRStateObjects.resize(8);

	for (int i = 0; i < 8; i++)
	{
		CD3DX12_STATE_OBJECT_DESC pipelineDesc{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

		InitShaders(&pipelineDesc, static_cast<EVRenderMode>(i));
		CreateHitGroups(&pipelineDesc);

		CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		UINT payloadSize = max(sizeof(VRayPayload), sizeof(VShadowRayPayload));
		UINT attributeSize = sizeof(VPrimitiveAttributes);

		shaderConfig->Config(payloadSize, attributeSize);

		CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSig = pipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		globalRootSig->SetRootSignature(GlobalRootSignature.Get());

		CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pipelineConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		pipelineConfig->Config(MAX_RAY_RECURSION_DEPTH);

		if (FAILED(Device->CreateStateObject(pipelineDesc, IID_PPV_ARGS(&DXRStateObjects[i]))))
		{
			V_LOG_ERROR("Failed to initialize raytracing pipeline!");
			return;
		}
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitShaders(CD3DX12_STATE_OBJECT_DESC* pipelineDesc, const EVRenderMode& renderMode)
{
	CD3DX12_DXIL_LIBRARY_SUBOBJECT* dxilLib = pipelineDesc->CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

	switch (renderMode)
	{
	case EVRenderMode::Interp:
	{
		D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));
		dxilLib->SetDXILLibrary(&shaderByteCode);
	}
		break;
	case EVRenderMode::Interp_Unlit:
	{
		D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing_unlit, ARRAYSIZE(g_pRaytracing_unlit));
		dxilLib->SetDXILLibrary(&shaderByteCode);
	}
		break;
	case EVRenderMode::Interp_NoTex:
	{
		D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing_noTex, ARRAYSIZE(g_pRaytracing_noTex));
		dxilLib->SetDXILLibrary(&shaderByteCode);
	}
		break;
	case EVRenderMode::Interp_NoTex_Unlit:
	{
		D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing_noTex_unlit, ARRAYSIZE(g_pRaytracing_noTex_unlit));
		dxilLib->SetDXILLibrary(&shaderByteCode);
	}
		break;
	case EVRenderMode::Cube:
	{
		D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing_cube, ARRAYSIZE(g_pRaytracing_cube));
		dxilLib->SetDXILLibrary(&shaderByteCode);
	}
		break;
	case EVRenderMode::Cube_Unlit:
	{
		D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing_cube_unlit, ARRAYSIZE(g_pRaytracing_cube_unlit));
		dxilLib->SetDXILLibrary(&shaderByteCode);
	}
		break;
	case EVRenderMode::Cube_NoTex:
	{
		D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing_cube_noTex, ARRAYSIZE(g_pRaytracing_cube_noTex));
		dxilLib->SetDXILLibrary(&shaderByteCode);
	}
		break;
	case EVRenderMode::Cube_NoTex_Unlit:
	{
		D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing_cube_noTex_unlit, ARRAYSIZE(g_pRaytracing_cube_noTex_unlit));
		dxilLib->SetDXILLibrary(&shaderByteCode);
	}
		break;
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::CreateHitGroups(CD3DX12_STATE_OBJECT_DESC* pipelineDesc)
{
	//Radiance rays
	CD3DX12_HIT_GROUP_SUBOBJECT* radianceHitGroup = pipelineDesc->CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();

	radianceHitGroup->SetIntersectionShaderImport(VDXConstants::SHADER_NAME_INTERSECTION.c_str());
	radianceHitGroup->SetClosestHitShaderImport(VDXConstants::SHADER_NAME_CLOSEST_HIT.c_str());
	radianceHitGroup->SetHitGroupExport(VDXConstants::HIT_GROUP.c_str());
	radianceHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);

	//Shadow rays
	CD3DX12_HIT_GROUP_SUBOBJECT* shadowHitGroup = pipelineDesc->CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();

	shadowHitGroup->SetIntersectionShaderImport(VDXConstants::SHADER_NAME_INTERSECTION_SHADOW.c_str());
	shadowHitGroup->SetHitGroupExport(VDXConstants::SHADOW_HIT_GROUP.c_str());
	shadowHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::CreateShaderTables()
{
	ShaderTableRayGen.resize(DXRStateObjects.size());
	ShaderTableMiss.resize(DXRStateObjects.size());
	ShaderTableHitGroups.resize(DXRStateObjects.size());
	StrideShaderTableHitGroups.resize(DXRStateObjects.size());
	StrideShaderTableMiss.resize(DXRStateObjects.size());

	for (int i = 0; i < DXRStateObjects.size(); i++)
	{
		CPtr<ID3D12StateObjectProperties> stateObjectProps;
		DXRStateObjects[i].As(&stateObjectProps);

		void* rayGenShaderID = stateObjectProps->GetShaderIdentifier(VDXConstants::SHADER_NAME_RAYGEN.c_str());
		void* missShaderID = stateObjectProps->GetShaderIdentifier(VDXConstants::SHADER_NAME_MISS.c_str());
		void* missShaderShadowID = stateObjectProps->GetShaderIdentifier(VDXConstants::SHADER_NAME_MISS_SHADOW.c_str());
		void* rayHitGroupID = stateObjectProps->GetShaderIdentifier(VDXConstants::HIT_GROUP.c_str());
		void* rayHitGroupShadowID = stateObjectProps->GetShaderIdentifier(VDXConstants::SHADOW_HIT_GROUP.c_str());

		UINT strideRayGen = 0;

		ShaderTableRayGen[i] = CreateShaderTable(std::vector<void*>{rayGenShaderID}, strideRayGen);
		ShaderTableMiss[i] = CreateShaderTable(std::vector<void*>{missShaderID, missShaderShadowID}, StrideShaderTableMiss[i]);
		ShaderTableHitGroups[i] = CreateShaderTable(std::vector<void*>{rayHitGroupID, rayHitGroupShadowID}, StrideShaderTableHitGroups[i]);
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::PrepareForRendering()
{
	UINT backBufferIndex = WindowRenderTarget->GetCurrentBufferIndex();

	WindowCommandAllocators[backBufferIndex]->Reset();
	CommandList->Reset(WindowCommandAllocators[backBufferIndex].Get(), nullptr);

	VDXAccelerationStructureBuffers* tlas = SceneToRender->GetAccelerationStructureTL(WindowRenderTarget->GetCurrentBufferIndex());
	CommandList->BuildRaytracingAccelerationStructure(&tlas->AccelerationStructureDesc, 0, nullptr);
	
	std::vector<D3D12_RESOURCE_BARRIER> barriers = {
		CD3DX12_RESOURCE_BARRIER::UAV(tlas->AccelerationStructure.Get()),
		CD3DX12_RESOURCE_BARRIER::Transition(WindowRenderTarget->GetCurrentRenderTarget().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)
	};

	CommandList->ResourceBarrier(barriers.size(), barriers.data());
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::DoRendering()
{
	boost::unordered_map<UINT, VDXResourceBindingPayload> resourceBindings;
	VDXAccelerationStructureBuffers* tlas = SceneToRender->GetAccelerationStructureTL(WindowRenderTarget->GetCurrentBufferIndex());

	FillDescriptorHeap(resourceBindings);

	CommandList->SetComputeRootSignature(GlobalRootSignature.Get());

	ID3D12DescriptorHeap* descHeaps[2];

	descHeaps[0] = RendererSamplerDescriptorHeap->GetDescriptorHeap().Get();
	descHeaps[1] = RendererDescriptorHeap->GetDescriptorHeap().Get();

	CommandList->SetDescriptorHeaps(2, &descHeaps[0]);
	CommandList->SetComputeRootConstantBufferView(EGlobalRootSignature::SceneConstant, SceneToRender->CopySceneConstantBufferToGPU(WindowRenderTarget->GetCurrentBufferIndex()));
	CommandList->SetComputeRootShaderResourceView(EGlobalRootSignature::AccelerationStructure, tlas->AccelerationStructure->GetGPUVirtualAddress());

	for (const auto& binding : resourceBindings)
	{
		CommandList->SetComputeRootDescriptorTable(binding.first, binding.second.BindingGPUHandle);
	}

	int renderMode = static_cast<int>(RenderMode);

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchDesc.HitGroupTable.StartAddress = ShaderTableHitGroups[renderMode]->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes = ShaderTableHitGroups[renderMode]->GetDesc().Width;
	dispatchDesc.HitGroupTable.StrideInBytes = StrideShaderTableHitGroups[renderMode];
	dispatchDesc.MissShaderTable.StartAddress = ShaderTableMiss[renderMode]->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes = ShaderTableMiss[renderMode]->GetDesc().Width;
	dispatchDesc.MissShaderTable.StrideInBytes = StrideShaderTableMiss[renderMode];
	dispatchDesc.RayGenerationShaderRecord.StartAddress = ShaderTableRayGen[renderMode]->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = ShaderTableRayGen[renderMode]->GetDesc().Width;
	dispatchDesc.Width = WindowRenderTarget->GetWidth();
	dispatchDesc.Height = WindowRenderTarget->GetHeight();
	dispatchDesc.Depth = 1;
	
	CommandList->SetPipelineState1(DXRStateObjects[renderMode].Get());
	CommandList->DispatchRays(&dispatchDesc);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::CopyRaytracingOutputToBackbuffer()
{
	D3D12_RESOURCE_BARRIER preCopyBarriers[2];
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(WindowRenderTarget->GetCurrentRenderTarget().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(WindowRenderTarget->GetCurrentOutputTexture().Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

	CommandList->ResourceBarrier(2, preCopyBarriers);
	CommandList->CopyResource(WindowRenderTarget->GetCurrentRenderTarget().Get(), WindowRenderTarget->GetCurrentOutputTexture().Get());

	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(WindowRenderTarget->GetCurrentRenderTarget().Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(WindowRenderTarget->GetCurrentOutputTexture().Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	CommandList->ResourceBarrier(2, postCopyBarriers);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::UploadPendingTexturesToGPU()
{
	bool hasTexturesToUpload = false;

	ID3D12GraphicsCommandList5* commandList = nullptr;

	std::vector<D3D12_RESOURCE_BARRIER> barriers;

	for (auto& textureUpload : TexturesToUpload)
	{
		if (!textureUpload.second.Texture.expired())
		{
			if (hasTexturesToUpload == false)
			{
				hasTexturesToUpload = true;

				commandList = UploadCommandHandler->StartCommandRecording();
			}

			VDXTextureUploadPayload& uploadPayload = textureUpload.second.UploadPayload;

			barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(uploadPayload.GPUBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		}
	}

	if (hasTexturesToUpload)
	{
		//commandList->ResourceBarrier(barriers.size(), barriers.data());

		for (auto& textureUpload : TexturesToUpload)
		{
			if (!textureUpload.second.Texture.expired())
			{
				VDXTextureUploadPayload& uploadPayload = textureUpload.second.UploadPayload;

				for (UINT64 subResourceIndex = 0; subResourceIndex < uploadPayload.SubResourceCount; subResourceIndex++)
				{
					commandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(uploadPayload.GPUBuffer.Get(), subResourceIndex), 0, 0, 0,
						&CD3DX12_TEXTURE_COPY_LOCATION(uploadPayload.UploadBuffer.Get(), uploadPayload.SubResourceFootprints[subResourceIndex]), nullptr);
				}
			}
		}

		UploadCommandHandler->ExecuteCommandQueue();
	}

	UploadCommandHandler->WaitForGPU();

	for (auto& textureUpload : TexturesToUpload)
	{
		if (!textureUpload.second.Texture.expired())
		{
			DXUsedTexture usedTexture; 
			usedTexture.Texture = textureUpload.second.Texture;
			usedTexture.Resource = textureUpload.second.UploadPayload.GPUBuffer;

			UploadedTextures[textureUpload.first] = usedTexture;

			textureUpload.second.Texture.lock()->EndGPUUpload();
		}
	}

	TexturesToUpload.clear();
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::ExecuteCommandList()
{
	CommandList->Close();

	ID3D12CommandList* commandLists[] = {
				CommandList.Get()
	};

	RenderCommandQueue->ExecuteCommandLists(1, commandLists);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::WaitForGPU()
{
	if (RenderCommandQueue && Fence && WindowRenderTarget != nullptr)
	{
		UINT64 fenceValue = WindowRenderTarget->GetCurrentFenceValue();

		RenderCommandQueue->Signal(Fence.Get(), fenceValue);
		WaitForFenceValue(Fence, fenceValue, FenceEvent);

		WindowRenderTarget->SetCurrentFenceValue(fenceValue + 1);
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::MoveToNextFrame()
{
	UINT64 currentFenceValue = WindowRenderTarget->GetCurrentFenceValue();
	RenderCommandQueue->Signal(Fence.Get(), currentFenceValue);
	
	WindowRenderTarget->SyncBackBufferIndexWithSwapChain();

	UINT64 nextBufferFenceValue = WindowRenderTarget->GetCurrentFenceValue();

	if (Fence->GetCompletedValue() < nextBufferFenceValue)
	{
		WaitForFenceValue(Fence, nextBufferFenceValue, FenceEvent);
	}

	WindowRenderTarget->SetCurrentFenceValue(currentFenceValue + 1);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::DeleteScene()
{
	if (SceneToRender != nullptr)
	{
		SceneToRender->Cleanup();

		delete SceneToRender;
		SceneToRender = nullptr;
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::FillDescriptorHeap(boost::unordered_map<UINT, VDXResourceBindingPayload>& outResourceBindings)
{
	UINT rangeIndex = 0;
	VDXResourceBindingPayload bindingPayload;

	unsigned int backBufferIndex = WindowRenderTarget->GetCurrentBufferIndex();

	RendererDescriptorHeap->AllocateDescriptorRange(1, rangeIndex);
	bindingPayload.BindingGPUHandle = RendererDescriptorHeap->GetGPUHandle(rangeIndex);

	outResourceBindings[EGlobalRootSignature::OutputView] = bindingPayload;

	Device->CopyDescriptorsSimple(1, RendererDescriptorHeap->GetCPUHandle(rangeIndex), WindowRenderTarget->GetOutputTextureCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	RendererDescriptorHeap->AllocateDescriptorRange(VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT + MaxAllowedObjectData * 3, rangeIndex);
	bindingPayload.BindingGPUHandle = RendererDescriptorHeap->GetGPUHandle(rangeIndex);

	outResourceBindings[EGlobalRootSignature::SceneTextures] = bindingPayload;

	Device->CopyDescriptorsSimple(VDXConstants::STATIC_SCENERY_SRV_CV_UAV_COUNT + MaxAllowedObjectData * 3, RendererDescriptorHeap->GetCPUHandle(rangeIndex), SceneToRender->GetSceneDescriptorHeap()->GetCPUHandle(0), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	RendererSamplerDescriptorHeap->AllocateDescriptorRange(VDXConstants::STATIC_SCENERY_SAMPLER_COUNT, rangeIndex);
	bindingPayload.BindingGPUHandle = RendererSamplerDescriptorHeap->GetGPUHandle(rangeIndex);

	outResourceBindings[EGlobalRootSignature::SceneSamplers] = bindingPayload;

	Device->CopyDescriptorsSimple(VDXConstants::STATIC_SCENERY_SAMPLER_COUNT, RendererSamplerDescriptorHeap->GetCPUHandle(rangeIndex), SceneToRender->GetSceneDescriptorHeapSamplers()->GetCPUHandle(0), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);


	RendererDescriptorHeap->AllocateDescriptorRange(MaxAllowedPointLights + MaxAllowedSpotLights, rangeIndex);
	bindingPayload.BindingGPUHandle = RendererDescriptorHeap->GetGPUHandle(rangeIndex);

	outResourceBindings[EGlobalRootSignature::SceneLights] = bindingPayload;

	Device->CopyDescriptorsSimple(MaxAllowedPointLights + MaxAllowedSpotLights, RendererDescriptorHeap->GetCPUHandle(rangeIndex), SceneToRender->GetSceneLightsHeapStart(backBufferIndex), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	RendererDescriptorHeap->AllocateDescriptorRange(VolumeRaytracer::MaxAllowedObjectData, rangeIndex);
	bindingPayload.BindingGPUHandle = RendererDescriptorHeap->GetGPUHandle(rangeIndex);

	outResourceBindings[EGlobalRootSignature::GeometryVolumes] = bindingPayload;

	Device->CopyDescriptorsSimple(VolumeRaytracer::MaxAllowedObjectData, RendererDescriptorHeap->GetCPUHandle(rangeIndex), SceneToRender->GetGeometrySRVDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	RendererDescriptorHeap->AllocateDescriptorRange(VolumeRaytracer::MaxAllowedObjectData, rangeIndex);
	bindingPayload.BindingGPUHandle = RendererDescriptorHeap->GetGPUHandle(rangeIndex);

	outResourceBindings[EGlobalRootSignature::GeometryTraversal] = bindingPayload;

	Device->CopyDescriptorsSimple(VolumeRaytracer::MaxAllowedObjectData, RendererDescriptorHeap->GetCPUHandle(rangeIndex), SceneToRender->GetGeometryTraversalDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	RendererDescriptorHeap->AllocateDescriptorRange(VolumeRaytracer::MaxAllowedObjectData, rangeIndex);
	bindingPayload.BindingGPUHandle = RendererDescriptorHeap->GetGPUHandle(rangeIndex);

	outResourceBindings[EGlobalRootSignature::GeometryConstants] = bindingPayload;

	Device->CopyDescriptorsSimple(VolumeRaytracer::MaxAllowedObjectData, RendererDescriptorHeap->GetCPUHandle(rangeIndex), SceneToRender->GetGeometryCBDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXRenderer::CreateShaderTable(std::vector<void*> shaderIdentifiers, UINT& outShaderTableSize, void* rootArguments /*= nullptr*/, const size_t& rootArgumentsSize /*= 0*/)
{
	CPtr<ID3D12Resource> shaderTableRes;

	UINT shaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + rootArgumentsSize;
	shaderTableEntrySize = (shaderTableEntrySize + 31) & ~31;

	UINT shaderTableSize = shaderTableEntrySize * shaderIdentifiers.size();

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(shaderTableSize, D3D12_RESOURCE_FLAG_NONE);

	Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&shaderTableRes));

	uint8_t* mappedData;
	CD3DX12_RANGE mappedRange(0, 0);

	shaderTableRes->Map(0, &mappedRange, reinterpret_cast<void**>(&mappedData));

	for (UINT i = 0; i < shaderIdentifiers.size(); i++)
	{
		uint8_t* dest = mappedData + (shaderTableEntrySize * i);

		memcpy(dest, shaderIdentifiers[i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		if (rootArguments != nullptr)
		{
			memcpy(dest + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, rootArguments, rootArgumentsSize);
		}
	}

	shaderTableRes->Unmap(0, nullptr);
	outShaderTableSize = shaderTableEntrySize;

	return shaderTableRes;
}

VolumeRaytracer::Renderer::DX::VDXRenderer::~VDXRenderer()
{
	DestroyRenderer();
}

VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::VDXWindowRenderTargetHandler(IDXGIFactory4* dxgiFactory, ID3D12Device5* dxDevice, ID3D12CommandQueue* commandQueue, HWND hwnd, const unsigned int& width, const unsigned int& height)
{
	CreateRenderTargets(dxgiFactory, dxDevice, commandQueue, hwnd, width, height);
}

VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::~VDXWindowRenderTargetHandler()
{
	if (RTVDescriptorHeap != nullptr)
	{
		delete RTVDescriptorHeap;
		RTVDescriptorHeap = nullptr;
	}

	if (ResourceHeap != nullptr)
	{
		delete ResourceHeap;
		ResourceHeap = nullptr;
	}

	for (auto& renderTarget : BackBufferArr)
	{
		if (renderTarget != nullptr)
		{
			renderTarget.Reset();
			renderTarget = nullptr;
		}
	}

	for (auto& texture : OutputTextureArr)
	{
		if (texture != nullptr)
		{
			texture.Reset();
			texture = nullptr;
		}
	}

	BackBufferArr.clear();
	OutputTextureArr.clear();

	if (SwapChain != nullptr)
	{
		SwapChain.Reset();
		SwapChain = nullptr;
	}
}

void VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::Resize(ID3D12Device5* dxDevice, const unsigned int& width, const unsigned int& height)
{
	if (SwapChain)
	{
		for (auto& renderTarget : BackBufferArr)
		{
			if (renderTarget != nullptr)
			{
				renderTarget.Reset();
				renderTarget = nullptr;
			}
		}

		SwapChain->ResizeBuffers(VDXConstants::BACK_BUFFER_COUNT, width, height, VDXConstants::BACK_BUFFER_FORMAT, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

		OutputHeight = height;
		OutputWidth = width;

		CreateBackBuffers(dxDevice);
	}
}

void VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::Present(const UINT& syncLevel, const UINT& flags)
{
	SwapChain->Present(syncLevel, flags);
}

void VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::SetCurrentBufferIndex(const unsigned int& bufferIndex)
{
	if(bufferIndex >= 0 && bufferIndex < VDXConstants::BACK_BUFFER_COUNT)
		CurrentBufferIndex = bufferIndex;
}

unsigned int VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::GetCurrentBufferIndex() const
{
	return CurrentBufferIndex;
}

unsigned int VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::GetWidth() const
{
	return OutputWidth;
}

unsigned int VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::GetHeight() const
{
	return OutputHeight;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::GetCurrentRenderTarget() const
{
	return BackBufferArr[GetCurrentBufferIndex()];
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::GetCurrentOutputTexture() const
{
	return OutputTextureArr[GetCurrentBufferIndex()];
}

UINT64 VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::GetCurrentFenceValue() const
{
	return FenceValues[GetCurrentBufferIndex()];
}

void VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::SetCurrentFenceValue(const UINT64& fenceValue)
{
	FenceValues[GetCurrentBufferIndex()] = fenceValue;
}

void VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::SyncBackBufferIndexWithSwapChain()
{
	SetCurrentBufferIndex(SwapChain->GetCurrentBackBufferIndex());
}

D3D12_CPU_DESCRIPTOR_HANDLE VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::GetOutputTextureCPUHandle() const
{
	return ResourceHeap->GetCPUHandle(GetCurrentBufferIndex());
}

void VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::CreateRenderTargets(IDXGIFactory4* dxgiFactory, ID3D12Device5* dxDevice, ID3D12CommandQueue* commandQueue, HWND hwnd, const unsigned int& width, const unsigned int& height)
{
	RTVDescriptorHeap = new VDXDescriptorHeap(dxDevice, VDXConstants::BACK_BUFFER_COUNT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	ResourceHeap = new VDXDescriptorHeap(dxDevice, VDXConstants::BACK_BUFFER_COUNT, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = VDXConstants::BACK_BUFFER_FORMAT;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = VDXConstants::BACK_BUFFER_COUNT;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = true;

	CPtr<IDXGISwapChain1> swapChain;

	if (FAILED(dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, &fsSwapChainDesc, nullptr, &swapChain)))
	{
		V_LOG_ERROR("Swap chain creation failed!");
		return;
	}

	swapChain.As(&SwapChain);
	dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	OutputWidth = width;
	OutputHeight = height;

	FenceValues.clear();
	BackBufferArr.clear();
	OutputTextureArr.clear();
	
	for (UINT i = 0; i < VDXConstants::BACK_BUFFER_COUNT; i++)
	{
		FenceValues.push_back(0);
		BackBufferArr.push_back(nullptr);
		OutputTextureArr.push_back(nullptr);
	}

	CurrentBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	SetCurrentFenceValue(1);

	CreateBackBuffers(dxDevice);
}

void VolumeRaytracer::Renderer::DX::VDXWindowRenderTargetHandler::CreateBackBuffers(ID3D12Device5* dxDevice)
{
	RTVDescriptorHeap->ResetAllocations();
	ResourceHeap->ResetAllocations();

	SyncBackBufferIndexWithSwapChain();

	for (UINT i = 0; i < VDXConstants::BACK_BUFFER_COUNT; i++)
	{
		FenceValues[i] = GetCurrentFenceValue();

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

		rtvDesc.Format = VDXConstants::BACK_BUFFER_FORMAT;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		if (BackBufferArr[i] != nullptr)
		{
			BackBufferArr[i].Reset();
		}

		SwapChain->GetBuffer(i, IID_PPV_ARGS(&BackBufferArr[i]));

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		UINT descIndex = 0;

		if(RTVDescriptorHeap->AllocateDescriptor(&cpuHandle, &gpuHandle, descIndex))
		{
			dxDevice->CreateRenderTargetView(BackBufferArr[i].Get(), &rtvDesc, cpuHandle);
		}

		if (OutputTextureArr[i] != nullptr)
		{
			OutputTextureArr[i].Reset();
			OutputTextureArr[i] = nullptr;
		}

		CD3DX12_RESOURCE_DESC outputTextDesc = CD3DX12_RESOURCE_DESC::Tex2D(VDXConstants::BACK_BUFFER_FORMAT, OutputWidth, OutputHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		dxDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &outputTextDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&OutputTextureArr[i]));

		if (ResourceHeap->AllocateDescriptor(&cpuHandle, &gpuHandle, descIndex))
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

			dxDevice->CreateUnorderedAccessView(OutputTextureArr[i].Get(), nullptr, &uavDesc, cpuHandle);
		}
	}
}

VolumeRaytracer::Renderer::DX::VDXGPUCommand::VDXGPUCommand(ID3D12Device5* device, const D3D12_COMMAND_LIST_TYPE& type, const std::string& debugName)
{
	InitializeCommandList(device, type, debugName);
}

VolumeRaytracer::Renderer::DX::VDXGPUCommand::~VDXGPUCommand()
{
	WaitForGPU();
	ReleaseInternalVariables();
}

ID3D12GraphicsCommandList5* VolumeRaytracer::Renderer::DX::VDXGPUCommand::StartCommandRecording()
{
	WaitForGPU();

	CommandAllocator->Reset();
	CommandList->Reset(CommandAllocator.Get(), nullptr);

	return CommandList.Get();
}

void VolumeRaytracer::Renderer::DX::VDXGPUCommand::ExecuteCommandQueue()
{
	CommandList->Close();

	ID3D12CommandList* commandLists[] = {
				CommandList.Get()
	};

	CommandQueue->ExecuteCommandLists(1, commandLists);

	FenceValue++;
	CommandQueue->Signal(Fence.Get(), FenceValue);
}

bool VolumeRaytracer::Renderer::DX::VDXGPUCommand::IsBusy() const
{
	return Fence && Fence->GetCompletedValue() != FenceValue;
}

void VolumeRaytracer::Renderer::DX::VDXGPUCommand::WaitForGPU()
{
	if (CommandQueue && Fence)
	{
		if (FenceValue != Fence->GetCompletedValue())
		{
			Fence->SetEventOnCompletion(FenceValue, FenceEvent);
			WaitForSingleObject(FenceEvent, INFINITE);
		}
	}
}

void VolumeRaytracer::Renderer::DX::VDXGPUCommand::InitializeCommandList(ID3D12Device5* device, const D3D12_COMMAND_LIST_TYPE& type, const std::string& debugName)
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};

	commandQueueDesc.Type = type;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	if (FAILED(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&CommandQueue))))
	{
		V_LOG_FATAL("Command queue creation failed!");
		return;
	}

	SetDXDebugName<ID3D12CommandQueue>(CommandQueue, debugName + "_QUEUE");

	if (FAILED(device->CreateCommandAllocator(type, IID_PPV_ARGS(&CommandAllocator))))
	{
		V_LOG_FATAL("Command allocator creation failed!");

		ReleaseInternalVariables();
		return;
	}

	SetDXDebugName<ID3D12CommandAllocator>(CommandAllocator, debugName + "_ALLOCATOR");

	if (FAILED(device->CreateCommandList(0, type, CommandAllocator.Get(), NULL, IID_PPV_ARGS(&CommandList))))
	{
		V_LOG_FATAL("Command list creation failed!");
		ReleaseInternalVariables();
		return;
	}

	SetDXDebugName<ID3D12CommandList>(CommandList, debugName + "_LIST");

	CommandList->Close();

	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence))))
	{
		V_LOG_FATAL("DX Fence creation failed!");
		ReleaseInternalVariables();
		return;
	}

	SetDXDebugName<ID3D12Fence>(Fence, debugName + "_FENCE");

	FenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
}

void VolumeRaytracer::Renderer::DX::VDXGPUCommand::ReleaseInternalVariables()
{
	if (Fence != nullptr)
	{
		CloseHandle(FenceEvent);

		Fence.Reset();
		Fence = nullptr;
	}

	if (CommandList != nullptr)
	{
		CommandList.Reset();
		CommandList = nullptr;
	}

	if (CommandAllocator != nullptr)
	{
		CommandAllocator.Reset();
		CommandAllocator = nullptr;
	}

	if (CommandQueue != nullptr)
	{
		CommandQueue.Reset();
		CommandQueue = nullptr;
	}
}
