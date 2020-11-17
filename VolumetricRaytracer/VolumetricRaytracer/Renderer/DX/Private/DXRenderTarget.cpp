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

#include "DXRenderTarget.h"
#include "Logger.h"
#include "d3dx12.h"
#include <dxgi1_4.h>

VolumeRaytracer::Renderer::DX::CPtr<IDXGISwapChain3> VolumeRaytracer::Renderer::DX::VDXRenderTarget::GetSwapChain() const
{
	return SwapChain;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12DescriptorHeap> VolumeRaytracer::Renderer::DX::VDXRenderTarget::GetViewHeapDesc() const
{
	return RenderTargetViewHeap;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12DescriptorHeap> VolumeRaytracer::Renderer::DX::VDXRenderTarget::GetResourceDescHeap() const
{
	return ResourceDescHeap;
}

std::vector<VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource>> VolumeRaytracer::Renderer::DX::VDXRenderTarget::GetBuffers() const
{
	return BufferArr;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXRenderTarget::GetCurrentBuffer() const
{
	return BufferArr[GetCurrentBufferIndex()];
}

unsigned int VolumeRaytracer::Renderer::DX::VDXRenderTarget::GetCurrentBufferIndex() const
{
	return BufferIndex;
}

void VolumeRaytracer::Renderer::DX::VDXRenderTarget::SetBufferIndex(const unsigned int& bufferIndex)
{
	if (bufferIndex < 0 || bufferIndex >= BufferArr.size())
	{
		V_LOG_ERROR("Buffer index is out of range");
	}
	else
	{
		BufferIndex = bufferIndex;
	}
}

unsigned int VolumeRaytracer::Renderer::DX::VDXRenderTarget::GetWidth() const
{
	return Width;
}

unsigned int VolumeRaytracer::Renderer::DX::VDXRenderTarget::GetHeight() const
{
	return Height;
}

void VolumeRaytracer::Renderer::DX::VDXRenderTarget::Release()
{
	VRenderTarget::Release();

	ReleaseInternalVariables();
}

bool VolumeRaytracer::Renderer::DX::VDXRenderTarget::AllocateNewResourceDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandle, unsigned int& descriptorIndex)
{
	if (NumResourceDescriptors >= ResourceDescHeapSize)
	{
		V_LOG_WARNING("Cant create render target resource! Heap is full!");
		return false;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE heapStart = ResourceDescHeap->GetCPUDescriptorHandleForHeapStart();
	*descriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapStart, NumResourceDescriptors, ResourceDescSize);

	descriptorIndex = NumResourceDescriptors;
	NumResourceDescriptors++;

	return true;
}

void VolumeRaytracer::Renderer::DX::VDXRenderTarget::ReleaseInternalVariables()
{
	if (OutputTexture != nullptr)
	{
		OutputTexture.Reset();
		OutputTexture = nullptr;
	}

	if (ResourceDescHeap != nullptr)
	{
		ResourceDescHeap.Reset();
		ResourceDescHeap = nullptr;
	}

	for (auto& buffer : BufferArr)
	{
		buffer.Reset();
		buffer = nullptr;
	}

	BufferArr.clear();

	if (RenderTargetViewHeap != nullptr)
	{
		RenderTargetViewHeap.Reset();
		RenderTargetViewHeap = nullptr;
	}

	if (SwapChain != nullptr)
	{
		SwapChain.Reset();
		SwapChain = nullptr;
	}
}

