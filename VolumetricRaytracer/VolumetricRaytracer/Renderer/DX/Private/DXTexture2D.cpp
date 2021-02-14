/*
	Copyright (c) 2021 Thomas Sch�ngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

#include "DXTexture2D.h"
#include <comdef.h>
#include "d3dx12.h"
#include "DXRenderer.h"
#include "MathHelpers.h"

VolumeRaytracer::Renderer::DX::VDXTexture2D::VDXTexture2D(const size_t& width, const size_t& height, const size_t& mipLevels)
{
	Width = width;
	Height = height;

	MipCount = mipLevels;
	PixelCount = Width * Height;

	if (width > 0 && height > 0 && mipLevels > 0)
	{
		Pixels = new uint8_t*[mipLevels];

		for (size_t i = 0; i < mipLevels; i++)
		{
			Pixels[i] = new uint8_t[PixelCount * 4];
		}
	}
}

void VolumeRaytracer::Renderer::DX::VDXTexture2D::GetPixels(const size_t& mipLevel, uint8_t*& outPixelArray, size_t* outArraySize)
{
	if (mipLevel >= 0 && mipLevel < GetMipCount())
	{
		outPixelArray = Pixels[mipLevel];
		*outArraySize = GetPixelCount() * 4;
	}
}

VolumeRaytracer::VColor VolumeRaytracer::Renderer::DX::VDXTexture2D::GetPixel(const size_t& x, const size_t& y, const size_t& mipLevel) const
{
	VColor res = VColor::BLACK;

	size_t index = VMathHelpers::Index3DTo1D(0, x, y, Width, Height);

	res.R = Pixels[mipLevel][index] / 255.f;
	res.G = Pixels[mipLevel][index + 1] / 255.f;
	res.B = Pixels[mipLevel][index + 2] / 255.f;
	res.A = Pixels[mipLevel][index + 3] / 255.f;

	return res;
}

void VolumeRaytracer::Renderer::DX::VDXTexture2D::SetPixel(const size_t& x, const size_t& y, const size_t& mipLevel, const VColor& pixelColor)
{
	size_t index = VMathHelpers::Index3DTo1D(0, x, y, Width, Height);

	Pixels[mipLevel][index] = 255 * pixelColor.R;
	Pixels[mipLevel][index + 1] = 255 * pixelColor.G;
	Pixels[mipLevel][index + 2] = 255 * pixelColor.B;
	Pixels[mipLevel][index + 3] = 255 * pixelColor.A;
}

void VolumeRaytracer::Renderer::DX::VDXTexture2D::Commit()
{
	
}

DXGI_FORMAT VolumeRaytracer::Renderer::DX::VDXTexture2D::GetDXGIFormat() const
{
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTexture2D::GetDXUploadResource() const
{
	return UploadResource;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTexture2D::GetDXGPUResource() const
{
	return GpuResource;
}

std::weak_ptr<VolumeRaytracer::Renderer::DX::VDXRenderer> VolumeRaytracer::Renderer::DX::VDXTexture2D::GetOwnerRenderer()
{
	return OwnerRenderer;
}

D3D12_SRV_DIMENSION VolumeRaytracer::Renderer::DX::VDXTexture2D::GetSRVDimension()
{
	return D3D12_SRV_DIMENSION_TEXTURE2D;
}

size_t VolumeRaytracer::Renderer::DX::VDXTexture2D::GetPixelCount()
{
	return PixelCount;
}

void VolumeRaytracer::Renderer::DX::VDXTexture2D::Initialize()
{
	
}

void VolumeRaytracer::Renderer::DX::VDXTexture2D::BeginDestroy()
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

void VolumeRaytracer::Renderer::DX::VDXTexture2D::SetOwnerRenderer(std::weak_ptr<VDXRenderer> renderer)
{
	OwnerRenderer = renderer;
}

void VolumeRaytracer::Renderer::DX::VDXTexture2D::InitGPUResource(VDXRenderer* renderer)
{
	CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(GetDXGIFormat(), GetWidth(),
		GetHeight(), 1, GetMipCount());

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	renderer->GetDXDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&GpuResource));

	SetDXDebugName<ID3D12Resource>(GpuResource, "2D Texture");
}

VolumeRaytracer::Renderer::DX::VDXTextureUploadPayload VolumeRaytracer::Renderer::DX::VDXTexture2D::BeginGPUUpload()
{
	if (!OwnerRenderer.expired())
	{
		CPtr<ID3D12Device5> dxDevice = OwnerRenderer.lock()->GetDXDevice();

		UINT64 numSubresources = GetMipCount();
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(GpuResource.Get(), 0, numSubresources);

		CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize, D3D12_RESOURCE_FLAG_NONE);

		CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

		dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadResource));

		SetDXDebugName<ID3D12Resource>(UploadResource, "Texture2D upload buffer");

		VDXTextureUploadPayload uploadPayload;
		uploadPayload.SubResourceFootprints.resize(numSubresources);

		UINT* rows = new UINT[numSubresources];
		UINT64* rowSizes = new UINT64[numSubresources];

		UINT64 totalSize = 0;

		dxDevice->GetCopyableFootprints(&GpuResource->GetDesc(), 0, GetMipCount(), 0, uploadPayload.SubResourceFootprints.data(), rows, rowSizes, &totalSize);

		if (UploadResource != nullptr)
		{
			uint8_t* uploadMemory;
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
				uint8_t* destSubResourceMemory = uploadMemory + layout.Offset;
				
				uint8_t* subResourceMemory = Pixels[mipIndex];

				for (UINT64 sliceIndex = 0; sliceIndex < subResourceDepth; sliceIndex++)
				{
					for (UINT64 height = 0; height < subResourceHeight; height++)
					{
						memcpy(destSubResourceMemory, subResourceMemory, (subResourcePitch < rawRowPitch) ? subResourcePitch : rawRowPitch);

						destSubResourceMemory += subResourcePitch;
						subResourceMemory += rawRowPitch;
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

void VolumeRaytracer::Renderer::DX::VDXTexture2D::EndGPUUpload()
{
	if (UploadResource != nullptr)
	{
		UploadResource.Reset();
		UploadResource = nullptr;
	}
}