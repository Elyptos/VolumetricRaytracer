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

#include "DXTexture3DFloat.h"
#include <comdef.h>
#include "d3dx12.h"
#include "DXRenderer.h"
#include "MathHelpers.h"

VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::VDXTexture3DFloat(const size_t& width, const size_t& height, const size_t& depth, const size_t& mipLevels)
{
	Width = width;
	Height = height;
	Depth = depth;

	MipCount = mipLevels;
	PixelCount = Width * Height * Depth;

	if (width > 0 && height > 0 && depth > 0 && mipLevels > 0)
	{
		Pixels = new float*[mipLevels];

		for (size_t i = 0; i < mipLevels; i++)
		{
			Pixels[i] = new float[PixelCount];
		}
	}
}

void VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::GetPixels(const size_t& mipLevel, float*& outPixelArray, size_t* outArraySize)
{
	if (mipLevel >= 0 && mipLevel < GetMipCount())
	{
		outPixelArray = Pixels[mipLevel];
		*outArraySize = GetPixelCount();
	}
}

void VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::Commit()
{

}

DXGI_FORMAT VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::GetDXGIFormat() const
{
	return DXGI_FORMAT_R32_FLOAT;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::GetDXUploadResource() const
{
	return UploadResource;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::GetDXGPUResource() const
{
	return GpuResource;
}

std::weak_ptr<VolumeRaytracer::Renderer::DX::VDXRenderer> VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::GetOwnerRenderer()
{
	return OwnerRenderer;
}

D3D12_SRV_DIMENSION VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::GetSRVDimension()
{
	return D3D12_SRV_DIMENSION_TEXTURE3D;
}

size_t VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::GetPixelCount()
{
	return PixelCount;
}

void VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::Initialize()
{

}

void VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::BeginDestroy()
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

	if (Pixels != nullptr)
	{
		for (size_t i = 0; i < GetMipCount(); i++)
		{
			delete[] Pixels[i];
		}

		delete[] Pixels;
		Pixels = nullptr;
	}
}

void VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::SetOwnerRenderer(std::weak_ptr<VDXRenderer> renderer)
{
	OwnerRenderer = renderer;
}

void VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::InitGPUResource(VDXRenderer* renderer)
{
	CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(GetDXGIFormat(), GetWidth(),
		GetHeight(), GetDepth(), GetMipCount());

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	renderer->GetDXDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&GpuResource));

	SetDXDebugName<ID3D12Resource>(GpuResource, "3D Texture Float");
}

VolumeRaytracer::Renderer::DX::VDXTextureUploadPayload VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::BeginGPUUpload()
{
	if (!OwnerRenderer.expired())
	{
		CPtr<ID3D12Device5> dxDevice = OwnerRenderer.lock()->GetDXDevice();

		UINT64 numSubresources = GetMipCount();
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(GpuResource.Get(), 0, numSubresources);

		CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize, D3D12_RESOURCE_FLAG_NONE);

		CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadResource));

		SetDXDebugName<ID3D12Resource>(UploadResource, "Texture3D upload buffer");

		VDXTextureUploadPayload uploadPayload;
		uploadPayload.SubResourceFootprints.resize(numSubresources);

		UINT* rows = new UINT[numSubresources];
		UINT64* rowSizes = new UINT64[numSubresources];

		UINT64 totalSize = 0;

		dxDevice->GetCopyableFootprints(&GpuResource->GetDesc(), 0, GetMipCount(), 0, uploadPayload.SubResourceFootprints.data(), rows, rowSizes, &totalSize);

		if (UploadResource != nullptr)
		{
			float* uploadMemory;
			CD3DX12_RANGE mapRange(0, 0);
			UploadResource->Map(0, &mapRange, reinterpret_cast<void**>(&uploadMemory));

			for (UINT64 mipIndex = 0; mipIndex < MipCount; mipIndex++)
			{
				UINT64 subResourceIndex = mipIndex;

				D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = uploadPayload.SubResourceFootprints[subResourceIndex];

				UINT64 subResourceHeight = rows[subResourceIndex];
				UINT64 subResourcePitch = VDXHelper::Align(layout.Footprint.RowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
				UINT64 rawRowPitch = rowSizes[subResourceIndex];
				UINT64 subResourceDepth = layout.Footprint.Depth;
				float* destSubResourceMemory = uploadMemory + layout.Offset;

				float* subResourceMemory = Pixels[mipIndex];

				for (UINT64 sliceIndex = 0; sliceIndex < subResourceDepth; sliceIndex++)
				{
					for (UINT64 height = 0; height < subResourceHeight; height++)
					{
						memcpy(destSubResourceMemory, subResourceMemory, (subResourcePitch < rawRowPitch) ? subResourcePitch : rawRowPitch);

						destSubResourceMemory = (float*)(((uint8_t*)destSubResourceMemory) + subResourcePitch);
						subResourceMemory = (float*)(((uint8_t*)subResourceMemory) + rawRowPitch);
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

void VolumeRaytracer::Renderer::DX::VDXTexture3DFloat::EndGPUUpload()
{
	if (UploadResource != nullptr)
	{
		UploadResource.Reset();
		UploadResource = nullptr;
	}
}
