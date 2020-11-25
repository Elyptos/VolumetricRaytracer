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
#include "DXRenderTarget.h"
#include "DXConstants.h"
#include "RaytracingHlsl.h"
#include "RDXScene.h"
#include "DXDescriptorHeap.h"
#include "Camera.h"
#include "DXTextureCube.h"
#include "Compiled/Raytracing.hlsl.h"
#include <string>
#include <sstream>
#include <vector>

void VolumeRaytracer::Renderer::DX::VDXRenderer::Render()
{
	if (IsActive())
	{
		if (SceneToRender != nullptr && !SceneRef.expired())
		{
			UploadPendingTexturesToGPU();

			for (auto elem : ActiveRenderTargets)
			{
				VDXRenderTarget* renderTarget = elem.first;

				SceneRef.lock()->GetSceneCamera()->AspectRatio = (float)renderTarget->GetWidth() / (float)renderTarget->GetHeight();
				SceneToRender->SyncWithScene(SceneRef.lock().get());
				PrepareForRendering(renderTarget);
				DoRendering(renderTarget);
				CopyRaytracingOutputToBackbuffer(renderTarget);

				ExecuteCommandList();

				renderTarget->SwapChain->Present(1, 0);

				WaitForGPU();

				if (renderTarget->GetCurrentBufferIndex() == 0)
				{
					renderTarget->SetBufferIndex(1);
				}
				else
				{
					renderTarget->SetBufferIndex(0);
				}
			}
		}

		//ClearAllRenderTargets();
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

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Renderer::DX::VDXRenderTarget> VolumeRaytracer::Renderer::DX::VDXRenderer::CreateViewportRenderTarget(HWND hwnd, unsigned int width, unsigned int height)
{
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	unsigned int renderTargetViewDescriptorSize = 0;
	unsigned int bufferCount = 0;

	V_LOG("Creating render target for window");

	VObjectPtr<VDXRenderTarget> res = VObject::CreateObject<VDXRenderTarget>();

	V_LOG("Creating Swapchain for render target");
	CreateSwapChain(hwnd, width, height, res->SwapChain);

	ZeroMemory(&renderTargetViewHeapDesc, sizeof(renderTargetViewHeapDesc));
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	if (FAILED(res->SwapChain->GetDesc(&swapChainDesc)))
	{
		V_LOG_ERROR("Unable to create dx render target for viewport! Unable to retrieve swap chain description!");

		return nullptr;
	}

	bufferCount = swapChainDesc.BufferCount;

	res->BufferCount = bufferCount;
	res->Width = width;
	res->Height = height;

	renderTargetViewHeapDesc.NumDescriptors = bufferCount;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	V_LOG("Creating render target buffers");

	if (FAILED(Device->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&res->RenderTargetViewHeap)))
	{
		V_LOG_ERROR("Unable to create dx render target for viewport!");

		return nullptr;
	}

	SetDXDebugName<ID3D12DescriptorHeap>(res->RenderTargetViewHeap, "Volume Raytracer View Heap");

	InitializeRenderTargetBuffers(res.get(), bufferCount);
	InitializeRenderTargetResourceDescHeap(res.get());
	CreateRenderingOutputTexture(res.get());

	V_LOG("Creating necessary pipeline variables for render target");

	AddRenderTargetToActiveMap(res.get());

	return res;
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::BuildAccelerationStructure(const VDXAccelerationStructureBuffers& topLevelAS, const VDXAccelerationStructureBuffers& bottomLevelAS)
{
	CommandAllocator->Reset();
	CommandList->Reset(CommandAllocator.Get(), nullptr);

	CommandList->BuildRaytracingAccelerationStructure(&bottomLevelAS.AccelerationStructureDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAS.AccelerationStructure.Get());
	CommandList->ResourceBarrier(1, &resourceBarrier);

	CommandList->BuildRaytracingAccelerationStructure(&topLevelAS.AccelerationStructureDesc, 0, nullptr);

	ExecuteCommandList();
	WaitForGPU();
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::SetSceneToRender(VObjectPtr<Voxel::VVoxelScene> scene)
{
	VRenderer::SetSceneToRender(scene);

	DeleteScene();

	SceneToRender = new VRDXScene();
	SceneToRender->InitFromScene(scene.get());
	SceneToRender->BuildStaticResources(this);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitializeTexture(VObjectPtr<VTextureCube> cubeTexture, bool uploadToGPU)
{
	VObjectPtr<VDXTextureCube> dxCubeTexture = std::dynamic_pointer_cast<VDXTextureCube>(cubeTexture);
	IDXRenderableTexture* renderableTexture = static_cast<IDXRenderableTexture*>(dxCubeTexture.get());

	if (dxCubeTexture != nullptr)
	{
		CPtr<ID3D12Resource> gpuResource = nullptr;

		CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(renderableTexture->GetDXGIFormat(), dxCubeTexture->GetWidth(),
			dxCubeTexture->GetHeight(), dxCubeTexture->GetArraySize(), dxCubeTexture->GetMipCount());

		CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&gpuResource));

		SetDXDebugName<ID3D12Resource>(gpuResource, "Cubemap");

		renderableTexture->SetDXGPUResource(gpuResource);
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::UploadToGPU(VObjectPtr<VTexture> texture)
{
	std::shared_ptr<IDXRenderableTexture> renderableTexture = std::dynamic_pointer_cast<IDXRenderableTexture>(texture);

	if (renderableTexture != nullptr && TexturesToUpload.find(renderableTexture.get()) == TexturesToUpload.end())
	{
		VDXTextureUploadPayload uploadPayload = renderableTexture->BeginGPUUpload(this);
		DXTextureUploadInfo uploadInfo;
		uploadInfo.Texture = renderableTexture;
		uploadInfo.UploadPayload = uploadPayload;

		TexturesToUpload[renderableTexture.get()] = uploadInfo;
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::MakeShaderResourceView(VObjectPtr<VTextureCube> texture)
{
	std::shared_ptr<IDXRenderableTexture> renderableTexture = std::dynamic_pointer_cast<IDXRenderableTexture>(texture);

	if (renderableTexture != nullptr)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = renderableTexture->GetCPUHandle();

		D3D12_SHADER_RESOURCE_VIEW_DESC resourceViewDesc = {};

		resourceViewDesc.Format = renderableTexture->GetDXGIFormat();
		resourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		resourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		resourceViewDesc.TextureCube.MostDetailedMip = 0;
		resourceViewDesc.TextureCube.MipLevels = texture->GetMipCount();
		resourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		Device->CreateShaderResourceView(renderableTexture->GetDXGPUResource().Get(), &resourceViewDesc, cpuHandle);
	}
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

		if (FAILED(Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&CommandQueue))))
		{
			V_LOG_FATAL("Command queue creation failed!");
			ReleaseInternalVariables();
			return;
		}

		SetDXDebugName<ID3D12CommandQueue>(CommandQueue, "VR Command Queue");

		if (FAILED(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator))))
		{
			V_LOG_FATAL("Command allocator creation failed!");
			ReleaseInternalVariables();
			return;
		}

		SetDXDebugName<ID3D12CommandAllocator>(CommandAllocator, "VR Command Allocator");

		if (FAILED(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator.Get(), NULL, IID_PPV_ARGS(&CommandList))))
		{
			V_LOG_FATAL("Command list creation failed!");
			ReleaseInternalVariables();
			return;
		}

		SetDXDebugName<ID3D12CommandList>(CommandList, "VR Command List");

		CommandList->Close();

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
		FenceValue = 1;

		IsInitialized = true;

		V_LOG("DirectX 12 renderer initialized!");
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::DestroyRenderer()
{
	if (IsActive())
	{
		V_LOG("Shutting down DirectX 12 renderer");

		ReleaseAllRenderTargets();
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
	//debugControllerExtended->SetEnableGPUBasedValidation(true);
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

void VolumeRaytracer::Renderer::DX::VDXRenderer::CreateSwapChain(HWND hwnd, unsigned int width, unsigned int height, CPtr<IDXGISwapChain3>& outSwapChain)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	CPtr<IDXGISwapChain1> swapChain = NULL;

	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = VDXConstants::BACK_BUFFER_FORMAT;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = 0;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc = {};
	fullScreenDesc.Windowed = true;

	if (FAILED(DXGIFactory->CreateSwapChainForHwnd(CommandQueue.Get(), hwnd, &swapChainDesc, &fullScreenDesc, nullptr, &swapChain)))
	{
		V_LOG_ERROR("Swap chain creation failed!");
		return;
	}

	swapChain.As(&outSwapChain);

	SetDXDebugName<IDXGISwapChain3>(outSwapChain, "Volume Raytracer Swapchain");
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitializeRenderTargetBuffers(VDXRenderTarget* renderTarget, const uint32_t& bufferCount)
{
	uint32_t renderTargetViewDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = renderTarget->RenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();;

	for (unsigned int i = 0; i < bufferCount; i++)
	{
		renderTarget->BufferArr.push_back(nullptr);

		if (FAILED(renderTarget->SwapChain->GetBuffer(i, IID_PPV_ARGS(&renderTarget->BufferArr[i]))))
		{
			V_LOG_FATAL("Unable to get swap chain buffer! This should never happen!");
			return;
		}

		SetDXDebugName<ID3D12Resource>(renderTarget->BufferArr[i], "Volume Raytracer Buffer");

		Device->CreateRenderTargetView(renderTarget->BufferArr[i].Get(), NULL, renderTargetViewHandle);
		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitializeRenderTargetResourceDescHeap(VDXRenderTarget* renderTarget)
{
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};

	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.NodeMask = 0;

	Device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&renderTarget->ResourceDescHeap));

	renderTarget->ResourceDescHeapSize = descHeapDesc.NumDescriptors;
	renderTarget->ResourceDescSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::CreateRenderingOutputTexture(VDXRenderTarget* renderTarget)
{
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(VDXConstants::BACK_BUFFER_FORMAT, renderTarget->GetWidth(), renderTarget->GetHeight(), 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	if (FAILED(Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&renderTarget->OutputTexture))))
	{
		V_LOG_ERROR("Failed to create output texture for render target");
		return;
	}

	SetDXDebugName(renderTarget->OutputTexture, "Volume Raytracer output texture");

	D3D12_CPU_DESCRIPTOR_HANDLE uavDescHandle;
	
	if (!renderTarget->AllocateNewResourceDescriptor(&uavDescHandle, renderTarget->OutputTextureDescIndex))
	{
		V_LOG_ERROR("Failed to bind output texture resource");

		renderTarget->OutputTexture.Reset();
		renderTarget->OutputTexture = nullptr;

		return;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	Device->CreateUnorderedAccessView(renderTarget->OutputTexture.Get(), nullptr, &uavDesc, uavDescHandle);
	renderTarget->OutputTextureGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(renderTarget->GetResourceDescHeap()->GetGPUDescriptorHandleForHeapStart(), renderTarget->OutputTextureDescIndex, renderTarget->ResourceDescSize);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::AddRenderTargetToActiveMap(VDXRenderTarget* renderTarget)
{
	DXRegisteredRenderTarget registrationContainer;

	registrationContainer.RenderTarget = renderTarget;
	registrationContainer.RenderTargetReleaseDelegateHandle = renderTarget->OnRenderTargetReleased_Bind(boost::bind(&VDXRenderer::OnRenderTargetPendingRelease, this, boost::placeholders::_1));

	ActiveRenderTargets[renderTarget] = registrationContainer;
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::RemoveRenderTargetFromActiveMap(VDXRenderTarget* renderTarget)
{
	if (ActiveRenderTargets.find(renderTarget) != ActiveRenderTargets.end())
	{
		DXRegisteredRenderTarget reg = ActiveRenderTargets[renderTarget];

		reg.RenderTargetReleaseDelegateHandle.disconnect();

		ActiveRenderTargets.erase(renderTarget);
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::OnRenderTargetPendingRelease(VRenderTarget* renderTarget)
{
	VDXRenderTarget* dxRenderTarget = dynamic_cast<VDXRenderTarget*>(renderTarget);
	ReleaseRenderTarget(dxRenderTarget);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::ReleaseRenderTarget(VDXRenderTarget* renderTarget)
{
	if (renderTarget != nullptr)
	{
		V_LOG("Trying to realse render target");

		if (ActiveRenderTargets.find(renderTarget) != ActiveRenderTargets.end())
		{
			V_LOG("Waiting for GPU to free Render Target");
			WaitForGPU();
			V_LOG("GPU finished. Now releasing Render Target");

			RemoveRenderTargetFromActiveMap(renderTarget);

			renderTarget->ReleaseInternalVariables();

			V_LOG("Render Target released!");
		}
		else
		{
			V_LOG_WARNING("Render target cannot be released by this renderer because it was created by another one!");
		}
	}
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

void VolumeRaytracer::Renderer::DX::VDXRenderer::ReleaseAllRenderTargets()
{
	V_LOG("Releasing all render targets!");

	std::vector<VDXRenderTarget*> renderTargets;

	for (auto elem : ActiveRenderTargets)
	{
		renderTargets.push_back(elem.first);
	}

	for (auto renderTarget : renderTargets)
	{
		ReleaseRenderTarget(renderTarget);
	}

	ActiveRenderTargets.clear();

	V_LOG("All render targets released!");
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::ReleaseInternalVariables()
{
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

	if (ShaderTableRayGen != nullptr)
	{
		ShaderTableRayGen.Reset();
		ShaderTableRayGen = nullptr;
	}

	if (ShaderTableMiss != nullptr)
	{
		ShaderTableMiss.Reset();
		ShaderTableMiss = nullptr;
	}

	if (ShaderTableHitGroups != nullptr)
	{
		ShaderTableHitGroups.Reset();
		ShaderTableHitGroups = nullptr;
	}

	if (Fence != nullptr)
	{
		CloseHandle(FenceEvent);

		Fence.Reset();
		Fence = nullptr;
	}

	FenceValue = 0;

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

void VolumeRaytracer::Renderer::DX::VDXRenderer::ClearAllRenderTargets()
{
	/*unsigned int renderTargetViewDescriptorSize = 0;
	renderTargetViewDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (auto elem : ActiveRenderTargets)
	{
		VDXRenderTarget* renderTarget = elem.first;

		CPtr<ID3D12Resource> buffer = renderTarget->GetBuffers()[renderTarget->GetBufferIndex()];

		CommandAllocator->Reset();
		CommandList->Reset(CommandAllocator.Get(), nullptr);

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		CommandList->ResourceBarrier(1, &barrier);

		float color[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(renderTarget->GetViewHeapDesc()->GetCPUDescriptorHandleForHeapStart(), renderTarget->GetBufferIndex(), renderTargetViewDescriptorSize);

		CommandList->ClearRenderTargetView(cpuHandle, color, 0, nullptr);

		barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		CommandList->ResourceBarrier(1, &barrier);
		CommandList->Close();

		ID3D12CommandList* commandLists[] = {
				CommandList.Get()
		};

		CommandQueue->ExecuteCommandLists(1, commandLists);

		renderTarget->FenceValue = SignalFence(CommandQueue, renderTarget->Fence, renderTarget->FenceValue);

		WaitForFenceValue(renderTarget->Fence, renderTarget->FenceValue, renderTarget->FenceEvent);

		unsigned int nextBufferIndex = renderTarget->GetBufferIndex() == 0u ? 1u : 0u;

		renderTarget->SetBufferIndex(nextBufferIndex);
		renderTarget->GetSwapChain()->Present(0, 0);
	}*/
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitRendererDescriptorHeap()
{
	if (RendererDescriptorHeap == nullptr)
	{
		RendererDescriptorHeap = new VDXDescriptorHeap(GetDXDevice(), 3);
		RendererDescriptorHeap->SetDebugName("Renderer Descriptor Heap");

		RendererSamplerDescriptorHeap = new VDXDescriptorHeap(GetDXDevice(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		RendererSamplerDescriptorHeap->SetDebugName("Renderer Sampler Descriptor Heap");
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitializeGlobalRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE outputViewDescRange = {};
	outputViewDescRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE sceneDescTable[2];
	sceneDescTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);
	sceneDescTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[EGlobalRootSignature::Max];
	rootParameters[EGlobalRootSignature::OutputView].InitAsDescriptorTable(1, &outputViewDescRange);
	rootParameters[EGlobalRootSignature::AccelerationStructure].InitAsShaderResourceView(0);
	rootParameters[EGlobalRootSignature::SceneConstant].InitAsConstantBufferView(0);
	rootParameters[EGlobalRootSignature::SceneTextures].InitAsDescriptorTable(1, &sceneDescTable[0]);
	rootParameters[EGlobalRootSignature::SceneSamplers].InitAsDescriptorTable(1, &sceneDescTable[1]);

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
	CD3DX12_STATE_OBJECT_DESC pipelineDesc { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	InitShaders(&pipelineDesc);
	CreateHitGroups(&pipelineDesc);

	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT payloadSize = max(sizeof(VRayPayload), sizeof(VShadowRayPayload));
	UINT attributeSize = sizeof(VPrimitiveAttributes);

	shaderConfig->Config(payloadSize, attributeSize);

	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSig = pipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSig->SetRootSignature(GlobalRootSignature.Get());

	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pipelineConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	pipelineConfig->Config(MAX_RAY_RECURSION_DEPTH);

	if (FAILED(Device->CreateStateObject(pipelineDesc, IID_PPV_ARGS(&DXRStateObject))))
	{
		V_LOG_ERROR("Failed to initialize raytracing pipeline!");
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::InitShaders(CD3DX12_STATE_OBJECT_DESC* pipelineDesc)
{
	CD3DX12_DXIL_LIBRARY_SUBOBJECT* dxilLib = pipelineDesc->CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

	D3D12_SHADER_BYTECODE shaderByteCode = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));
	dxilLib->SetDXILLibrary(&shaderByteCode);
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

	shadowHitGroup->SetIntersectionShaderImport(VDXConstants::SHADER_NAME_INTERSECTION.c_str());
	shadowHitGroup->SetHitGroupExport(VDXConstants::SHADOW_HIT_GROUP.c_str());
	shadowHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::CreateShaderTables()
{
	CPtr<ID3D12StateObjectProperties> stateObjectProps;
	DXRStateObject.As(&stateObjectProps);

	void* rayGenShaderID = stateObjectProps->GetShaderIdentifier(VDXConstants::SHADER_NAME_RAYGEN.c_str());
	void* missShaderID = stateObjectProps->GetShaderIdentifier(VDXConstants::SHADER_NAME_MISS.c_str());
	void* missShaderShadowID = stateObjectProps->GetShaderIdentifier(VDXConstants::SHADER_NAME_MISS_SHADOW.c_str());
	void* rayHitGroupID = stateObjectProps->GetShaderIdentifier(VDXConstants::HIT_GROUP.c_str());
	void* rayHitGroupShadowID = stateObjectProps->GetShaderIdentifier(VDXConstants::SHADOW_HIT_GROUP.c_str());

	UINT strideRayGen = 0;

	ShaderTableRayGen = CreateShaderTable(std::vector<void*>{rayGenShaderID}, strideRayGen);
	ShaderTableMiss = CreateShaderTable(std::vector<void*>{missShaderID, missShaderShadowID}, StrideShaderTableMiss);
	ShaderTableHitGroups = CreateShaderTable(std::vector<void*>{rayHitGroupID, rayHitGroupShadowID}, StrideShaderTableHitGroups);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::PrepareForRendering(VDXRenderTarget* renderTarget)
{
	CommandAllocator->Reset();
	CommandList->Reset(CommandAllocator.Get(), nullptr);

	FillDescriptorHeap(renderTarget);

	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget->GetCurrentBuffer().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->ResourceBarrier(1, &barrier);

	SceneToRender->BuildVoxelVolume(CommandList);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::DoRendering(VDXRenderTarget* renderTarget)
{
	CommandList->SetComputeRootSignature(GlobalRootSignature.Get());

	ID3D12DescriptorHeap* descHeaps[2];

	descHeaps[0] = RendererDescriptorHeap->GetDescriptorHeap().Get();
	descHeaps[1] = RendererSamplerDescriptorHeap->GetDescriptorHeap().Get();

	CommandList->SetDescriptorHeaps(2, &descHeaps[0]);
	CommandList->SetComputeRootDescriptorTable(EGlobalRootSignature::OutputView, RendererDescriptorHeap->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
	CommandList->SetComputeRootConstantBufferView(EGlobalRootSignature::SceneConstant, SceneToRender->CopySceneConstantBufferToGPU());

	CommandList->SetComputeRootShaderResourceView(EGlobalRootSignature::AccelerationStructure, SceneToRender->GetAccelerationStructureTL()->GetGPUVirtualAddress());
	CommandList->SetComputeRootDescriptorTable(EGlobalRootSignature::SceneTextures, RendererDescriptorHeap->GetGPUHandle(1));
	CommandList->SetComputeRootDescriptorTable(EGlobalRootSignature::SceneSamplers, RendererSamplerDescriptorHeap->GetGPUHandle(0));

	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchDesc.HitGroupTable.StartAddress = ShaderTableHitGroups->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes = ShaderTableHitGroups->GetDesc().Width;
	dispatchDesc.HitGroupTable.StrideInBytes = StrideShaderTableHitGroups;
	dispatchDesc.MissShaderTable.StartAddress = ShaderTableMiss->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes = ShaderTableMiss->GetDesc().Width;
	dispatchDesc.MissShaderTable.StrideInBytes = StrideShaderTableMiss;
	dispatchDesc.RayGenerationShaderRecord.StartAddress = ShaderTableRayGen->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = ShaderTableRayGen->GetDesc().Width;
	dispatchDesc.Width = renderTarget->Width;
	dispatchDesc.Height = renderTarget->Height;
	dispatchDesc.Depth = 1;
	
	CommandList->SetPipelineState1(DXRStateObject.Get());
	CommandList->DispatchRays(&dispatchDesc);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::CopyRaytracingOutputToBackbuffer(VDXRenderTarget* renderTarget)
{
	D3D12_RESOURCE_BARRIER preCopyBarriers[2];
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget->GetCurrentBuffer().Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget->OutputTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

	CommandList->ResourceBarrier(2, preCopyBarriers);
	CommandList->CopyResource(renderTarget->GetCurrentBuffer().Get(), renderTarget->OutputTexture.Get());

	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget->GetCurrentBuffer().Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget->OutputTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	CommandList->ResourceBarrier(2, postCopyBarriers);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::UploadPendingTexturesToGPU()
{
	bool hasTexturesToUpload = false;

	for (auto& textureUpload : TexturesToUpload)
	{
		if (!textureUpload.second.Texture.expired())
		{
			if (hasTexturesToUpload == false)
			{
				hasTexturesToUpload = true;

				CommandAllocator->Reset();
				CommandList->Reset(CommandAllocator.Get(), nullptr);
			}

			VDXTextureUploadPayload& uploadPayload = textureUpload.second.UploadPayload;

			for (UINT64 subResourceIndex = 0; subResourceIndex < uploadPayload.SubResourceCount; subResourceIndex++)
			{
				CommandList->CopyTextureRegion(&CD3DX12_TEXTURE_COPY_LOCATION(uploadPayload.GPUBuffer.Get(), subResourceIndex), 0, 0, 0, 
					&CD3DX12_TEXTURE_COPY_LOCATION(uploadPayload.UploadBuffer.Get(), uploadPayload.SubResourceFootprints[subResourceIndex]), nullptr);
			}
		}
	}

	if (hasTexturesToUpload)
	{
		ExecuteCommandList();
		WaitForGPU();
	}

	for (auto& textureUpload : TexturesToUpload)
	{
		if (!textureUpload.second.Texture.expired())
		{
			DXUsedTexture usedTexture; 
			usedTexture.Texture = textureUpload.second.Texture;
			usedTexture.Resource = textureUpload.second.UploadPayload.GPUBuffer;

			UploadedTextures[textureUpload.first] = usedTexture;
		}
	}

	TexturesToUpload.empty();
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::ExecuteCommandList()
{
	CommandList->Close();

	ID3D12CommandList* commandLists[] = {
				CommandList.Get()
	};

	CommandQueue->ExecuteCommandLists(1, commandLists);
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::WaitForGPU()
{
	if (CommandQueue && Fence)
	{
		uint64_t fenceValueForSignal = SignalFence(CommandQueue, Fence, FenceValue);
		WaitForFenceValue(Fence, fenceValueForSignal, FenceEvent);
	}
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

void VolumeRaytracer::Renderer::DX::VDXRenderer::FillDescriptorHeap(VDXRenderTarget* renderTarget)
{
	//D3D12_CPU_DESCRIPTOR_HANDLE destDescriptors[]{ RendererDescriptorHeap->GetCPUHandle(0)/*, RendererDescriptorHeap->GetCPUHandle(0)*/ };
	//UINT destDescriptorSizes[]{/*RendererDescriptorHeap->GetDescriptorSize(),*/ RendererDescriptorHeap->GetDescriptorSize()};

	//D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors[]{ renderTarget->ResourceDescHeap->GetCPUDescriptorHandleForHeapStart()/*, SceneToRender->GetSceneDescriptorHeap()->GetCPUHandle(0)*/ };
	//UINT srcDescriptorSizes[]{/*renderTarget->ResourceDescSize,*/ SceneToRender->GetSceneDescriptorHeap()->GetDescriptorSize()};

	//Device->CopyDescriptors(1, &destDescriptors[0], &destDescriptorSizes[0], 1, &srcDescriptors[0], &srcDescriptorSizes[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	Device->CopyDescriptorsSimple(1, RendererDescriptorHeap->GetCPUHandle(0), renderTarget->ResourceDescHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	Device->CopyDescriptorsSimple(2, RendererDescriptorHeap->GetCPUHandle(1), SceneToRender->GetSceneDescriptorHeap()->GetCPUHandle(0), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	Device->CopyDescriptorsSimple(1, RendererSamplerDescriptorHeap->GetCPUHandle(0), SceneToRender->GetSceneDescriptorHeapSamplers()->GetCPUHandle(0), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
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

	return shaderTableRes;
}

VolumeRaytracer::Renderer::DX::VDXRenderer::~VDXRenderer()
{
	DestroyRenderer();
}
