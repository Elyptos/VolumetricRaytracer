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

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Renderer::DX::VDXTextureCube> VolumeRaytracer::Renderer::DX::VDXTextureCube::LoadFromFile(const std::wstring& path)
{
	DirectX::ScratchImage* imageData = new DirectX::ScratchImage();

	HRESULT loadingRes = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, *imageData);

	if (FAILED(loadingRes))
	{
		std::string errorMessage = _com_error(loadingRes).ErrorMessage();

		V_LOG_ERROR("Texture loading failed! " + errorMessage);
		delete imageData;

		return nullptr;
	}

	const DirectX::TexMetadata& metadata = imageData->GetMetadata();

	if (metadata.IsCubemap())
	{
		VObjectPtr<VDXTextureCube> res = VObject::CreateObject<VDXTextureCube>();

		res->AssetPath = path;
		res->Width = metadata.width;
		res->Height = metadata.height;
		res->ArraySize = metadata.arraySize;
		res->DXTextureFormat = metadata.format;
		res->MipCount = metadata.mipLevels;
		res->RawImage = imageData;

		return res;
	}
	else
	{
		V_LOG_ERROR("Loaded texture is not a cubemap!");
		delete imageData;

		return nullptr;
	}
}

DXGI_FORMAT VolumeRaytracer::Renderer::DX::VDXTextureCube::GetDXGIFormat() const
{
	return DXTextureFormat;
}

DirectX::ScratchImage* VolumeRaytracer::Renderer::DX::VDXTextureCube::GetRawData() const
{
	return RawImage;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTextureCube::GetDXUploadResource() const 
{
	return UploadResource;
}

VolumeRaytracer::Renderer::DX::CPtr<ID3D12Resource> VolumeRaytracer::Renderer::DX::VDXTextureCube::GetDXGPUResource() const 
{
	return GpuResource;
}

D3D12_CPU_DESCRIPTOR_HANDLE VolumeRaytracer::Renderer::DX::VDXTextureCube::GetCPUHandle() const
{
	return CPUDescHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE VolumeRaytracer::Renderer::DX::VDXTextureCube::GetGPUHandle() const
{
	return GPUDescHandle;
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::SetDescriptorHandles(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
{
	CPUDescHandle = cpuHandle;
	GPUDescHandle = gpuHandle;
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
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::SetDXUploadResource(CPtr<ID3D12Resource> resource)
{
	UploadResource = resource;
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::SetDXGPUResource(CPtr<ID3D12Resource> resource)
{
	GpuResource = resource;
}

VolumeRaytracer::Renderer::DX::VDXTextureUploadPayload VolumeRaytracer::Renderer::DX::VDXTextureCube::BeginGPUUpload(VDXRenderer* renderer)
{
	CPtr<ID3D12Device5> dxDevice = renderer->GetDXDevice();

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

	return VDXTextureUploadPayload();
}

void VolumeRaytracer::Renderer::DX::VDXTextureCube::EndGPUUpload(VDXRenderer* renderer)
{
	if (UploadResource != nullptr)
	{
		UploadResource.Reset();
		UploadResource = nullptr;
	}
}