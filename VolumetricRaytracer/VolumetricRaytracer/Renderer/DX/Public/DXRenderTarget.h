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

#include "RenderTarget.h"
#include "DXHelper.h"
#include <d3d12.h>
#include <vector>

struct IDXGISwapChain3;

namespace VolumeRaytracer
{
	namespace Renderer
	{
		namespace DX
		{
			class VDXRenderTarget : public VRenderTarget
			{
				friend class VDXRenderer;

			public:
				CPtr<IDXGISwapChain3> GetSwapChain() const;
				CPtr<ID3D12DescriptorHeap> GetViewHeapDesc() const;
				CPtr<ID3D12DescriptorHeap> GetResourceDescHeap() const;

				std::vector<CPtr<ID3D12Resource>> GetBuffers() const;
				CPtr<ID3D12Resource> GetCurrentBuffer() const;
				unsigned int GetCurrentBufferIndex() const;
				void SetBufferIndex(const unsigned int& bufferIndex);

				unsigned int GetWidth() const;
				unsigned int GetHeight() const;

				void Release() override;

				bool AllocateNewResourceDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandle, unsigned int& descriptorIndex);

			protected:
				void ReleaseInternalVariables();

			private:
				std::vector<CPtr<ID3D12Resource>> BufferArr;

				CPtr<ID3D12DescriptorHeap> RenderTargetViewHeap = nullptr;

				CPtr<ID3D12DescriptorHeap> ResourceDescHeap = nullptr;
				unsigned int ResourceDescHeapSize = 0;
				unsigned int NumResourceDescriptors = 0;
				unsigned int ResourceDescSize = 0;

				CPtr<IDXGISwapChain3> SwapChain = nullptr;

				CPtr<ID3D12Resource> OutputTexture = nullptr;
				unsigned int OutputTextureDescIndex = 0;
				D3D12_GPU_DESCRIPTOR_HANDLE OutputTextureGPUHandle;

				unsigned int BufferIndex = 0;
				unsigned int Width = 0;
				unsigned int Height = 0;
			};
		}
	}
}