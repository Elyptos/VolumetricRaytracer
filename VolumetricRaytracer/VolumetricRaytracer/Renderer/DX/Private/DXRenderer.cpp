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
#include "d3dx12.h"
#include <string>
#include <sstream>
#include <vector>

void VolumeRaytracer::Renderer::DX::VDXRenderer::Render()
{
	if (IsActive())
	{
		ClearAllRenderTargets();
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

	V_LOG("Creating necessary pipeline variables for render target");

	if (FAILED(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&res->Fence))))
	{
		V_LOG_FATAL("DX Fence creation failed!");

		return false;
	}

	SetDXDebugName<ID3D12Fence>(res->Fence, "Volume Raytracer Fence");

	res->FenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	res->FenceValue = 1;

	AddRenderTargetToActiveMap(res.get());

	return res;
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
	debugControllerExtended->SetEnableGPUBasedValidation(true);
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
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	CPtr<IDXGISwapChain> swapChain = NULL;

	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = hwnd;

	swapChainDesc.Windowed = true;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	swapChainDesc.Flags = 0;

	if (FAILED(DXGIFactory->CreateSwapChain(CommandQueue.Get(), &swapChainDesc, &swapChain)))
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
			if (renderTarget->Fence != nullptr)
			{
				V_LOG("Waiting for GPU to free Render Target");
				FlushRenderTarget(renderTarget);
				V_LOG("GPU finished. Now releasing Render Target");

				CloseHandle(renderTarget->FenceEvent);
				RemoveRenderTargetFromActiveMap(renderTarget);

				renderTarget->ReleaseInternalVariables();

				V_LOG("Render Target released!");
			}
		}
		else
		{
			V_LOG_WARNING("Render target cannot be released by this renderer because it was created by another one!");
		}
	}
}

void VolumeRaytracer::Renderer::DX::VDXRenderer::FlushRenderTarget(VDXRenderTarget* renderTarget)
{
	if (renderTarget->Fence != nullptr)
	{
		uint64_t fenceValueForSignal = SignalFence(CommandQueue, renderTarget->Fence, renderTarget->FenceValue);
		WaitForFenceValue(renderTarget->Fence, fenceValueForSignal, renderTarget->FenceEvent);
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
	unsigned int renderTargetViewDescriptorSize = 0;
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
	}
}

VolumeRaytracer::Renderer::DX::VDXRenderer::~VDXRenderer()
{
	DestroyRenderer();
}
