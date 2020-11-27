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

#include "Object.h"
#include "d3dx12.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include "DXHelper.h"

namespace VolumeRaytracer
{
	namespace Renderer
	{
		namespace DX
		{
			class VDXDescriptorHeap
			{
			public:
				VDXDescriptorHeap() = default;
				VDXDescriptorHeap(CPtr<ID3D12Device5> dxDevice, const UINT& maxDescriptors, const D3D12_DESCRIPTOR_HEAP_TYPE& heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, const D3D12_DESCRIPTOR_HEAP_FLAGS& heapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
				~VDXDescriptorHeap();

				void SetDebugName(const std::string& name);
				UINT GetDescriptorSize() const;

				bool AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* outHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle, UINT& outDescriptorIndex);

				CPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const { return DescHeap;}

				D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(const UINT& index);
				D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const UINT& index);

				void ResetAllocations();

			private:
				void Reset();
				void Init(CPtr<ID3D12Device5> dxDevice, const UINT& maxDescriptors, const D3D12_DESCRIPTOR_HEAP_TYPE& heapType, const D3D12_DESCRIPTOR_HEAP_FLAGS& heapFlags);

			private:
				CPtr<ID3D12DescriptorHeap> DescHeap = nullptr;

				UINT ResourceDescSize = 0;
				UINT MaxNumberOfDescriptors = 0;
				UINT NumAllocatedDescriptors = 0;
				
			};
		}
	}
}