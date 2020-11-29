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

#include "DXTextureCube.h"
#include "Logger.h"
#include "DirectXTex.h"
#include "DXRenderer.h"
#include <comdef.h>
#include <cmath>

DXGI_FORMAT VolumeRaytracer::Renderer::DX::VDXTextureCube::GetDXGIFormat() const
{
	return DXTextureFormat;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTextureCube::GetDXUploadResource() const 
{
	return UploadResource;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTextureCube::GetDXGPUResource() const 
{
	return GpuResource;
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::GetPixels(const size_t& mipLevel, uint8_t*& outPixelArray, size_t* outArraySize)
{
	if (RawImage != nullptr && mipLevel >= 0 && mipLevel < GetMipCount())
	{
		const DirectX::Image* image = RawImage->GetImage(mipLevel, 0, 0);

		if (image != nullptr)
		{
			outPixelArray = image->pixels;
			*outArraySize = image->width * image->height;

			return;
		}
	}

	outPixelArray = nullptr;
	outArraySize = 0;
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::Commit()
{
	
}

std::weak_ptr<VolumeRaytracer::Renderer::DX::VDXRenderer> VolumeRaytracer::Renderer::DX::VDXTextureCube::GetOwnerRenderer()
{
	return OwnerRenderer;
}

D3D12_SRV_DIMENSION VolumeRaytracer::Renderer::DX::VDXTextureCube::GetSRVDimension()
{
	return D3D12_SRV_DIMENSION_TEXTURECUBE;
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::Initialize()
{
	
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::BeginDestroy()
{
	if (RawImage != nullptr)
	{
		delete RawImage;
		RawImage = nullptr;
	}

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

VolumeRaytracer::Renderer::DX::VDXTextureUploadPayload VolumeRaytracer::Renderer::DX::VDXTextureCube::BeginGPUUpload()
{
	if (!OwnerRenderer.expired())
	{
		CPtr<ID3D12Device5> dxDevice = OwnerRenderer.lock()->GetDXDevice();

		UINT64 numSubresources = GetMipCount() * GetArraySize();
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(GpuResource.Get(), 0, numSubresources);

		CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize, D3D12_RESOURCE_FLAG_NONE);

		CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadResource));

		SetDXDebugName<ID3D12Resource>(UploadResource, "Cubemap upload buffer");

		VDXTextureUploadPayload uploadPayload;
		uploadPayload.SubResourceFootprints.resize(numSubresources);

		UINT* rows = new UINT[numSubresources];
		UINT64* rowSizes = new UINT64[numSubresources];

		UINT64 totalSize = 0;

		dxDevice->GetCopyableFootprints(&GpuResource->GetDesc(), 0, GetMipCount() * GetArraySize(), 0, uploadPayload.SubResourceFootprints.data(), rows, rowSizes, &totalSize);

		if (UploadResource != nullptr)
		{
			uint8_t* uploadMemory;
			CD3DX12_RANGE mapRange(0, 0);
			UploadResource->Map(0, &mapRange, reinterpret_cast<void**>(&uploadMemory));

			for (UINT64 arrayIndex = 0; arrayIndex < ArraySize; arrayIndex++)
			{
				for (UINT64 mipIndex = 0; mipIndex < MipCount; mipIndex++)
				{
					UINT64 subResourceIndex = mipIndex + (arrayIndex * MipCount);

					D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = uploadPayload.SubResourceFootprints[subResourceIndex];

					UINT64 subResourceHeight = rows[subResourceIndex];
					UINT64 subResourcePitch = VDXHelper::Align(layout.Footprint.RowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
					UINT64 subResourceDepth = layout.Footprint.Depth;
					uint8_t* destSubResourceMemory = uploadMemory + layout.Offset;

					for (UINT64 sliceIndex = 0; sliceIndex < subResourceDepth; sliceIndex++)
					{
						const DirectX::Image* rawSubImage = RawImage->GetImage(mipIndex, arrayIndex, sliceIndex);
						uint8_t* subResourceMemory = rawSubImage->pixels;

						for (UINT64 height = 0; height < subResourceHeight; height++)
						{
							memcpy(destSubResourceMemory, subResourceMemory, (subResourcePitch < rawSubImage->rowPitch) ? subResourcePitch : rawSubImage->rowPitch);

							destSubResourceMemory += subResourcePitch;
							subResourceMemory += rawSubImage->rowPitch;
						}
					}
				}
			}

			delete[] rowSizes;
			delete[] rows;

			UploadResource->Unmap(0, nullptr);

			uploadPayload.GPUBuffer = GpuResource;
			uploadPayload.UploadBuffer = UploadResource;
			uploadPayload.SubResourceCount = numSubresources;

			return uploadPayload;
		}
	}
		
	return VDXTextureUploadPayload();
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::EndGPUUpload()
{
	if (UploadResource != nullptr)
	{
		UploadResource.Reset();
		UploadResource = nullptr;
	}
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::InitGPUResource(VDXRenderer* renderer)
{
	CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(GetDXGIFormat(), GetWidth(),
		GetHeight(), GetArraySize(), GetMipCount());

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	renderer->GetDXDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&GpuResource));

	SetDXDebugName<ID3D12Resource>(GpuResource, "Cubemap");
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::SetOwnerRenderer(std::weak_ptr<VDXRenderer> renderer)
{
	OwnerRenderer = renderer;
}
