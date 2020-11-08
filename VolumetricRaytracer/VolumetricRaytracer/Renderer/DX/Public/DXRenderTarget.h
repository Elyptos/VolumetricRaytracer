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
#include <vector>

struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct IDXGISwapChain3;
struct ID3D12Fence;

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
				std::vector<CPtr<ID3D12Resource>> GetBuffers() const;


				void Release() override;

			protected:
				void ReleaseInternalVariables();

			private:
				std::vector<CPtr<ID3D12Resource>> BufferArr;
				CPtr<ID3D12DescriptorHeap> RenderTargetViewHeap = nullptr;
				CPtr<IDXGISwapChain3> SwapChain = nullptr;
				CPtr<ID3D12Fence> Fence = nullptr;
				HANDLE FenceEvent;
				uint64_t FenceValue = 0;
			};
		}
	}
}