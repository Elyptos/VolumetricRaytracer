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

#include "DXDescriptorHeap.h"
#include "Logger.h"

VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::VDXDescriptorHeap(CPtr<ID3D12Device5> dxDevice, const UINT& maxDescriptors, const D3D12_DESCRIPTOR_HEAP_TYPE& heapType /*= D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV*/, const D3D12_DESCRIPTOR_HEAP_FLAGS& heapFlags /*= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE*/)
{
	DescHeap = nullptr;
	Init(dxDevice, maxDescriptors, heapType, heapFlags);
}

VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::~VDXDescriptorHeap()
{
	Reset();
}

void VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::SetDebugName(const std::string& name)
{
	SetDXDebugName<ID3D12DescriptorHeap>(GetDescriptorHeap(), name);
}

UINT VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::GetDescriptorSize() const
{
	return ResourceDescSize;
}

void VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::Reset()
{
	MaxNumberOfDescriptors = 0;
	ResourceDescSize = 0;
	NumAllocatedDescriptors = 0;

	if (AllocatedDescriptors != nullptr)
	{
		delete[] AllocatedDescriptors;
		AllocatedDescriptors = nullptr;
	}

	if (DescHeap)
	{
		DescHeap.Reset();
		DescHeap = nullptr;
	}
}

void VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::Init(CPtr<ID3D12Device5> dxDevice, const UINT& maxDescriptors, const D3D12_DESCRIPTOR_HEAP_TYPE& heapType, const D3D12_DESCRIPTOR_HEAP_FLAGS& heapFlags)
{
	Reset();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	MaxNumberOfDescriptors = maxDescriptors;
	ResourceDescSize = dxDevice->GetDescriptorHandleIncrementSize(heapType);

	heapDesc.Type = heapType;
	heapDesc.Flags = heapFlags;
	heapDesc.NumDescriptors = MaxNumberOfDescriptors;
	heapDesc.NodeMask = 0;

	dxDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DescHeap));
	AllocatedDescriptors = new D3D12_CPU_DESCRIPTOR_HANDLE[MaxNumberOfDescriptors];
}

bool VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* outHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle, UINT& outDescriptorIndex)
{
	if (MaxNumberOfDescriptors <= NumAllocatedDescriptors)
	{
		V_LOG_WARNING("Resource descriptor heap is full!");
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE heapStart = DescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHeapStart = DescHeap->GetGPUDescriptorHandleForHeapStart();

	*outHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapStart, NumAllocatedDescriptors, ResourceDescSize);
	*outGpuHandle =	CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuHeapStart, NumAllocatedDescriptors, ResourceDescSize);

	outDescriptorIndex = NumAllocatedDescriptors;
	NumAllocatedDescriptors++;

	AllocatedDescriptors[outDescriptorIndex] = *outHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::GetCPUHandle(const UINT& index)
{
	if (index < 0 || index >= MaxNumberOfDescriptors)
	{
		return D3D12_CPU_DESCRIPTOR_HANDLE();
	}
	else
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(DescHeap->GetCPUDescriptorHandleForHeapStart(), index, ResourceDescSize);
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE VolumeRaytracer::Renderer::DX::VDXDescriptorHeap::GetGPUHandle(const UINT& index)
{
	if (index < 0 || index >= MaxNumberOfDescriptors)
	{
		return D3D12_GPU_DESCRIPTOR_HANDLE();
	}
	else
	{
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(DescHeap->GetGPUDescriptorHandleForHeapStart(), index, ResourceDescSize);
	}
}
