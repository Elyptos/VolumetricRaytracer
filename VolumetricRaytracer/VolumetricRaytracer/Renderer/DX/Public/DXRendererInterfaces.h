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

#include "DXHelper.h"
#include <memory>
#include <vector>

namespace DirectX
{
	class ScratchImage;
}

namespace VolumeRaytracer
{
	namespace Renderer
	{
		namespace DX
		{
			struct VDXTextureUploadPayload
			{
			public:
				CPtr<ID3D12Resource> UploadBuffer;
				CPtr<ID3D12Resource> GPUBuffer;
				UINT64 SubResourceCount;

				std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> SubResourceFootprints;
			};

			class IDXRenderableTexture
			{
				friend class VDXRenderer;

			public:
				virtual ~IDXRenderableTexture() = default;

				virtual DirectX::ScratchImage* GetRawData() const = 0;
				virtual DXGI_FORMAT GetDXGIFormat() const = 0;
				virtual CPtr<ID3D12Resource> GetDXUploadResource() const = 0;
				virtual CPtr<ID3D12Resource> GetDXGPUResource() const = 0;
				virtual D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const = 0;
				virtual D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const = 0;

				virtual void SetDescriptorHandles(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle) = 0;

			protected:
				virtual void SetDXUploadResource(CPtr<ID3D12Resource> resource) = 0;
				virtual void SetDXGPUResource(CPtr<ID3D12Resource> resource) = 0;

				virtual VDXTextureUploadPayload BeginGPUUpload(VDXRenderer* renderer) = 0;
				virtual void EndGPUUpload(VDXRenderer* renderer) = 0;
			};
		}
	}
}