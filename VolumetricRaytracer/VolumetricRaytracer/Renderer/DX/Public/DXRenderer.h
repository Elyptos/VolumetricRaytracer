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

#include "Renderer.h"
#include "DXHelper.h"
#include "Object.h"
#include "DXShaderTypes.h"
#include "DXRendererInterfaces.h"
#include "d3dx12.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <windef.h>
#include <memory>

#include <boost/unordered_map.hpp>
#include <boost/signals2.hpp>

namespace VolumeRaytracer
{
	namespace Renderer
	{
		class VRenderTarget;

		namespace DX
		{
			class VDXRenderTarget;
			class VRDXScene;
			class VDXDescriptorHeap;
			class VDXDescriptorHeapRingBuffer;

			struct VDXResourceBindingPayload
			{
			public:
				D3D12_GPU_DESCRIPTOR_HANDLE BindingGPUHandle;
			};

			class VDXWindowRenderTargetHandler
			{
			public:
				VDXWindowRenderTargetHandler(IDXGIFactory4* dxgiFactory, ID3D12Device5* dxDevice, ID3D12CommandQueue* commandQueue, HWND hwnd, const unsigned int& width, const unsigned int& height);
				~VDXWindowRenderTargetHandler();

				void Resize(ID3D12Device5* dxDevice, const unsigned int& width, const unsigned int& height);
				void Present(const UINT& syncLevel, const UINT& flags);

				void SetCurrentBufferIndex(const unsigned int& bufferIndex);
				unsigned int GetCurrentBufferIndex() const;

				unsigned int GetWidth() const;
				unsigned int GetHeight() const;

				CPtr<ID3D12Resource> GetCurrentRenderTarget() const;
				CPtr<ID3D12Resource> GetCurrentOutputTexture() const;
				UINT64 GetCurrentFenceValue() const;

				void SetCurrentFenceValue(const UINT64& fenceValue);
				void SyncBackBufferIndexWithSwapChain();

				D3D12_CPU_DESCRIPTOR_HANDLE GetOutputTextureCPUHandle() const;

			private:
				void CreateRenderTargets(IDXGIFactory4* dxgiFactory, ID3D12Device5* dxDevice, ID3D12CommandQueue* commandQueue, HWND hwnd, const unsigned int& width, const unsigned int& height);
				void CreateBackBuffers(ID3D12Device5* dxDevice);

			public:
				VDXDescriptorHeap* RTVDescriptorHeap = nullptr;
				VDXDescriptorHeap* ResourceHeap = nullptr;

				CPtr<IDXGISwapChain3> SwapChain = nullptr;

				std::vector<CPtr<ID3D12Resource>> BackBufferArr;
				std::vector<CPtr<ID3D12Resource>> OutputTextureArr;
				std::vector<UINT64> FenceValues;

				unsigned int OutputWidth = 0;
				unsigned int OutputHeight = 0;
				unsigned int CurrentBufferIndex = 0;
			};

			class VDXGPUCommand
			{
			public:
				VDXGPUCommand(ID3D12Device5* device, const D3D12_COMMAND_LIST_TYPE& type, const std::string& debugName);
				~VDXGPUCommand();

				ID3D12GraphicsCommandList5* StartCommandRecording();
				void ExecuteCommandQueue();
				bool IsBusy() const;
				void WaitForGPU();

			private:
				void InitializeCommandList(ID3D12Device5* device, const D3D12_COMMAND_LIST_TYPE& type, const std::string& debugName);

				void ReleaseInternalVariables();

			private:
				CPtr<ID3D12CommandQueue> CommandQueue = nullptr;
				CPtr<ID3D12GraphicsCommandList5> CommandList = nullptr;
				CPtr<ID3D12CommandAllocator> CommandAllocator = nullptr;

				HANDLE FenceEvent;
				CPtr<ID3D12Fence> Fence = nullptr;
				UINT64 FenceValue = 0;
			};

			class VDXRenderer : public VRenderer
			{
			private:
				struct DXRegisteredRenderTarget
				{
				public:
					VDXRenderTarget* RenderTarget;
					boost::signals2::connection RenderTargetReleaseDelegateHandle;
				};

				struct DXTextureUploadInfo
				{
				public:
					std::weak_ptr<IDXRenderableTexture> Texture;
					VDXTextureUploadPayload UploadPayload;
				};

				struct DXUsedTexture
				{
				public:
					std::weak_ptr<IDXRenderableTexture> Texture;
					CPtr<ID3D12Resource> Resource;
				};

			public:
				VDXRenderer() = default;
				~VDXRenderer();

				void Render() override;
				bool Start() override;
				void Stop() override;

				bool IsActive() const override { return IsInitialized; }

				void SetWindowHandle(HWND hwnd, unsigned int width, unsigned int height);
				void ClearWindowHandle();

				void ResizeRenderOutput(unsigned int width, unsigned int height) override;

				CPtr<ID3D12Device5> GetDXDevice() const { return Device; }

				void BuildBottomLevelAccelerationStructure(std::vector<VDXAccelerationStructureBuffers> bottomLevelAS);
				
				void SetSceneToRender(VObjectPtr<Scene::VScene> scene) override;


				void InitializeTexture(VObjectPtr<VTexture> texture) override;

				void CreateSRVDescriptor(VObjectPtr<VTexture> texture, const D3D12_CPU_DESCRIPTOR_HANDLE& descHandle);
				void CreateCBDescriptor(CPtr<ID3D12Resource> resource, const size_t& resourceSize, const D3D12_CPU_DESCRIPTOR_HANDLE& descHandle);

				void UploadToGPU(VObjectPtr<VTexture> texture) override;

				bool HasValidWindow() const;

			private:
				bool SetupRenderer();
				void DestroyRenderer();

				void SetupDebugLayer();
				void SetupDebugQueue();

				CPtr<IDXGIAdapter3> SelectGPU();

				uint64_t SignalFence(CPtr<ID3D12CommandQueue> commandQueue, CPtr<ID3D12Fence> fence, uint64_t& fenceValue);
				void WaitForFenceValue(CPtr<ID3D12Fence> fence, const uint64_t fenceValue, HANDLE fenceEvent);

				void ReleaseWindowResources();

				void ReleaseInternalVariables();

				void InitRendererDescriptorHeap();
				void InitializeGlobalRootSignature();
				void InitRaytracingPipeline();
				void InitShaders(CD3DX12_STATE_OBJECT_DESC* pipelineDesc, const EVRenderMode& renderMode);
				void CreateHitGroups(CD3DX12_STATE_OBJECT_DESC* pipelineDesc);

				void CreateShaderTables();

				void PrepareForRendering();
				void DoRendering();
				void CopyRaytracingOutputToBackbuffer();

				void UploadPendingTexturesToGPU();

				void ExecuteCommandList();
				void WaitForGPU();

				void MoveToNextFrame();

				void DeleteScene();


				void FillDescriptorHeap(boost::unordered_map<UINT, VDXResourceBindingPayload>& outResourceBindings);

				CPtr<ID3D12Resource> CreateShaderTable(std::vector<void*> shaderIdentifiers, UINT& outShaderTableSize, void* rootArguments = nullptr, const size_t& rootArgumentsSize = 0);

			private:
				//Window resources
				std::vector<CPtr<ID3D12CommandAllocator>> WindowCommandAllocators;
				VDXWindowRenderTargetHandler* WindowRenderTarget = nullptr;

				//Command lists
				VDXGPUCommand* UploadCommandHandler = nullptr;
				VDXGPUCommand* ComputeCommandHandler = nullptr;

				CPtr<ID3D12Device5> Device;

				CPtr<ID3D12CommandQueue> RenderCommandQueue;

				CPtr<IDXGIFactory4> DXGIFactory;
				CPtr<ID3D12GraphicsCommandList5> CommandList;

				std::vector<CPtr<ID3D12StateObject>> DXRStateObjects;
				CPtr<ID3D12PipelineState> PipelineState;

				CPtr<ID3D12Fence> Fence = nullptr;
				HANDLE FenceEvent;

				std::vector<CPtr<ID3D12Resource>> ShaderTableRayGen;
				std::vector<CPtr<ID3D12Resource>> ShaderTableHitGroups;
				std::vector<UINT> StrideShaderTableHitGroups;
				std::vector<CPtr<ID3D12Resource>> ShaderTableMiss;
				std::vector<UINT> StrideShaderTableMiss;

				VDXDescriptorHeapRingBuffer* RendererDescriptorHeap = nullptr;
				VDXDescriptorHeapRingBuffer* RendererSamplerDescriptorHeap = nullptr;

				CPtr<ID3D12RootSignature> GlobalRootSignature;
				boost::unordered_map<EShaderType, CPtr<ID3D12RootSignature>> LocalRootSignatures;

				bool IsInitialized = false;

				VRDXScene* SceneToRender = nullptr;

				boost::unordered_map<IDXRenderableTexture*, DXTextureUploadInfo> TexturesToUpload;
				boost::unordered_map<IDXRenderableTexture*, DXUsedTexture> UploadedTextures; 
			};
		}
	}
}