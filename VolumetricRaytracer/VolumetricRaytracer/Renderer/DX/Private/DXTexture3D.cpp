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

#include "DXTexture3D.h"
#include <comdef.h>
#include "d3dx12.h"
#include "DXRenderer.h"

void VolumeRaytracer::Renderer::DX::VDXTexture3D::GetPixels(const size_t& mipLevel, uint8_t*& outPixelArray, size_t* outArraySize)
{
	
}

void VolumeRaytracer::Renderer::DX::VDXTexture3D::Commit()
{
	
}

DXGI_FORMAT VolumeRaytracer::Renderer::DX::VDXTexture3D::GetDXGIFormat() const
{
	return DXGI_FORMAT_R8G8B8A8_UINT;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTexture3D::GetDXUploadResource() const
{
	return UploadResource;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTexture3D::GetDXGPUResource() const
{
	return GpuResource;
}

std::weak_ptr<VolumeRaytracer::Renderer::DX::VDXRenderer> VolumeRaytracer::Renderer::DX::VDXTexture3D::GetOwnerRenderer()
{
	return OwnerRenderer;
}

D3D12_SRV_DIMENSION VolumeRaytracer::Renderer::DX::VDXTexture3D::GetSRVDimension()
{
	return D3D12_SRV_DIMENSION_TEXTURE3D;
}

void VolumeRaytracer::Renderer::DX::VDXTexture3D::Initialize()
{
	
}

void VolumeRaytracer::Renderer::DX::VDXTexture3D::BeginDestroy()
{
	if (UploadResource != nullptr)
	{
		UploadResource.Reset();
		UploadResource = nullptr;
	}

	if (GpuResource != nullptr)
	{
		GpuResource.Reset();
		GpuResource = nullptr;
	}
}

void VolumeRaytracer::Renderer::DX::VDXTexture3D::SetOwnerRenderer(std::weak_ptr<VDXRenderer> renderer)
{
	OwnerRenderer = renderer;
}

void VolumeRaytracer::Renderer::DX::VDXTexture3D::InitGPUResource(VDXRenderer* renderer)
{
	CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(GetDXGIFormat(), GetWidth(),
		GetHeight(), GetDepth(), GetMipCount());

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	renderer->GetDXDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&GpuResource));

	SetDXDebugName<ID3D12Resource>(GpuResource, "3D Texture");
}

VolumeRaytracer::Renderer::DX::VDXTextureUploadPayload VolumeRaytracer::Renderer::DX::VDXTexture3D::BeginGPUUpload()
{
	return VDXTextureUploadPayload();
}

void VolumeRaytracer::Renderer::DX::VDXTexture3D::EndGPUUpload()
{
	
}
