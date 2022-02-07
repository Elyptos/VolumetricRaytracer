/*
	Copyright (c) 2020 Thomas Sch�ngrundner

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
#include "Textures/Texture3DFloat.h"
#include "DXRendererInterfaces.h"

namespace VolumeRaytracer
{
	namespace Renderer
	{
		namespace DX
		{
			class VDXTexture3DFloat : public VTexture3DFloat, public IDXRenderableTexture
			{
				friend class VTextureFactory;

			public:
				VDXTexture3DFloat() = default;
				VDXTexture3DFloat(const size_t& width, const size_t& height, const size_t& depth, const size_t& mipLevels);

				void GetPixels(const size_t& mipLevel, float*& outPixelArray, size_t* outArraySize) override;

				void Commit() override;

				DXGI_FORMAT GetDXGIFormat() const override;


				CPtr<ID3D12Resource> GetDXUploadResource() const override;


				CPtr<ID3D12Resource> GetDXGPUResource() const override;

				std::weak_ptr<VDXRenderer> GetOwnerRenderer() override;


				D3D12_SRV_DIMENSION GetSRVDimension() override;


				size_t GetPixelCount() override;

			protected:
				void Initialize() override;


				void BeginDestroy() override;


				void SetOwnerRenderer(std::weak_ptr<VDXRenderer> renderer) override;


				void InitGPUResource(VDXRenderer* renderer) override;


				VDXTextureUploadPayload BeginGPUUpload() override;


				void EndGPUUpload() override;

			private:
				CPtr<ID3D12Resource> UploadResource = nullptr;
				CPtr<ID3D12Resource> GpuResource = nullptr;

				float** Pixels = nullptr;

				size_t PixelCount = 0;

				std::weak_ptr<VDXRenderer> OwnerRenderer;
			};
		}
	}
}