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

#include "TextureCube.h"
#include "DXRendererInterfaces.h"



namespace VolumeRaytracer
{
	namespace Renderer
	{
		namespace DX
		{
			class VDXTextureCube : public VTextureCube, public IDXRenderableTexture
			{
			public:
				static VObjectPtr<VDXTextureCube> LoadFromFile(const std::wstring& path);

				DirectX::ScratchImage* GetRawData() const override;

				DXGI_FORMAT GetDXGIFormat() const override;
				CPtr<ID3D12Resource> GetDXUploadResource() const override;
				CPtr<ID3D12Resource> GetDXGPUResource() const override;


				D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const override;


				D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const override;


				void SetDescriptorHandles(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, const D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle) override;

			protected:
				void Initialize() override;

				void BeginDestroy() override;

				void SetDXUploadResource(CPtr<ID3D12Resource> resource) override;

				void SetDXGPUResource(CPtr<ID3D12Resource> resource) override;

				VDXTextureUploadPayload BeginGPUUpload(VDXRenderer* renderer) override;

				void EndGPUUpload(VDXRenderer* renderer) override;

			private:
				DXGI_FORMAT DXTextureFormat;
				DirectX::ScratchImage* RawImage = nullptr;
				CPtr<ID3D12Resource> UploadResource = nullptr;
				CPtr<ID3D12Resource> GpuResource = nullptr;

				D3D12_CPU_DESCRIPTOR_HANDLE CPUDescHandle;
				D3D12_GPU_DESCRIPTOR_HANDLE GPUDescHandle;
			};
		}
	}
}