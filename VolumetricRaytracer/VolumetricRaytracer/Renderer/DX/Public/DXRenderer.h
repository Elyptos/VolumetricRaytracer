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

			class VDXRenderer : public VRenderer
			{
			private:
				struct DXRegisteredRenderTarget
				{
				public:
					VDXRenderTarget* RenderTarget;
					boost::signals2::connection RenderTargetReleaseDelegateHandle;
				};

			public:
				VDXRenderer() = default;
				~VDXRenderer();

				void Render() override;
				void Start() override;
				void Stop() override;

				bool IsActive() const override { return IsInitialized; }

				VObjectPtr<VDXRenderTarget> CreateViewportRenderTarget(HWND hwnd, unsigned int width, unsigned int height);

				CPtr<ID3D12Device5> GetDXDevice() const { return Device; }

				void BuildAccelerationStructure(const VDXAccelerationStructureBuffers& topLevelAS, const VDXAccelerationStructureBuffers& bottomLevelAS);

				void SetSceneToRender(VObjectPtr<Voxel::VVoxelScene> scene) override;

			private:
				void SetupRenderer();
				void DestroyRenderer();

				void SetupDebugLayer();
				void SetupDebugQueue();

				CPtr<IDXGIAdapter3> SelectGPU();

				void CreateSwapChain(HWND hwnd, unsigned int width, unsigned int height, CPtr<IDXGISwapChain3>& outSwapChain);
				void InitializeRenderTargetBuffers(VDXRenderTarget* renderTarget, const uint32_t& bufferCount);
				void InitializeRenderTargetResourceDescHeap(VDXRenderTarget* renderTarget);
				void CreateRenderingOutputTexture(VDXRenderTarget* renderTarget);

				void AddRenderTargetToActiveMap(VDXRenderTarget* renderTarget);
				void RemoveRenderTargetFromActiveMap(VDXRenderTarget* renderTarget);

				void OnRenderTargetPendingRelease(VRenderTarget* renderTarget);
				void ReleaseRenderTarget(VDXRenderTarget* renderTarget);

				uint64_t SignalFence(CPtr<ID3D12CommandQueue> commandQueue, CPtr<ID3D12Fence> fence, uint64_t& fenceValue);
				void WaitForFenceValue(CPtr<ID3D12Fence> fence, const uint64_t fenceValue, HANDLE fenceEvent);

				void ReleaseAllRenderTargets();

				void ReleaseInternalVariables();

				void ClearAllRenderTargets();

				void InitializeGlobalRootSignature();
				void InitRaytracingPipeline();
				void InitShaders(CD3DX12_STATE_OBJECT_DESC* pipelineDesc);
				void CreateHitGroups(CD3DX12_STATE_OBJECT_DESC* pipelineDesc);

				void CreateShaderTables();

				void PrepareForRendering(VDXRenderTarget* renderTarget);
				void DoRendering(VDXRenderTarget* renderTarget);
				void CopyRaytracingOutputToBackbuffer(VDXRenderTarget* renderTarget);

				void ExecuteCommandList();
				void WaitForGPU();

				void DeleteScene();

				CPtr<ID3D12Resource> CreateShaderTable(std::vector<void*> shaderIdentifiers, UINT& outShaderTableSize, void* rootArguments = nullptr, const size_t& rootArgumentsSize = 0);

			private:
				CPtr<ID3D12Device5> Device;
				CPtr<ID3D12CommandQueue> CommandQueue;
				CPtr<IDXGIFactory4> DXGIFactory;
				CPtr<ID3D12CommandAllocator> CommandAllocator;
				CPtr<ID3D12GraphicsCommandList5> CommandList;
				CPtr<ID3D12StateObject> DXRStateObject;
				CPtr<ID3D12PipelineState> PipelineState;

				CPtr<ID3D12Fence> Fence = nullptr;
				HANDLE FenceEvent;
				uint64_t FenceValue = 0;

				CPtr<ID3D12Resource> ShaderTableRayGen;
				CPtr<ID3D12Resource> ShaderTableHitGroups;
				UINT StrideShaderTableHitGroups;
				CPtr<ID3D12Resource> ShaderTableMiss;
				UINT StrideShaderTableMiss;

				CPtr<ID3D12Resource> OutputTexture;
				D3D12_GPU_DESCRIPTOR_HANDLE OutputTextureHandle;

				CPtr<ID3D12RootSignature> GlobalRootSignature;
				boost::unordered_map<EShaderType, CPtr<ID3D12RootSignature>> LocalRootSignatures;

				boost::unordered_map<VDXRenderTarget*, DXRegisteredRenderTarget> ActiveRenderTargets;

				bool IsInitialized = false;

				VRDXScene* SceneToRender = nullptr;
			};
		}
	}
}