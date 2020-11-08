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

			private:
				void SetupRenderer();
				void DestroyRenderer();

				void SetupDebugLayer();
				void SetupDebugQueue();

				CPtr<IDXGIAdapter3> SelectGPU();

				void CreateSwapChain(HWND hwnd, unsigned int width, unsigned int height, CPtr<IDXGISwapChain3>& outSwapChain);
				void InitializeRenderTargetBuffers(VDXRenderTarget* renderTarget, const uint32_t& bufferCount);

				void AddRenderTargetToActiveMap(VDXRenderTarget* renderTarget);
				void RemoveRenderTargetFromActiveMap(VDXRenderTarget* renderTarget);

				void OnRenderTargetPendingRelease(VRenderTarget* renderTarget);
				void ReleaseRenderTarget(VDXRenderTarget* renderTarget);

				void FlushRenderTarget(VDXRenderTarget* renderTarget);

				uint64_t SignalFence(CPtr<ID3D12CommandQueue> commandQueue, CPtr<ID3D12Fence> fence, uint64_t& fenceValue);
				void WaitForFenceValue(CPtr<ID3D12Fence> fence, const uint64_t fenceValue, HANDLE fenceEvent);

				void ReleaseAllRenderTargets();

				void ReleaseInternalVariables();

			private:
				CPtr<ID3D12Device5> Device;
				CPtr<ID3D12CommandQueue> CommandQueue;
				CPtr<IDXGIFactory4> DXGIFactory;
				CPtr<ID3D12CommandAllocator> CommandAllocator;
				CPtr<ID3D12GraphicsCommandList4> CommandList;
				CPtr<ID3D12PipelineState> PipelineState;

				boost::unordered_map<VDXRenderTarget*, DXRegisteredRenderTarget> ActiveRenderTargets;

				bool IsInitialized = false;
			};
		}
	}
}