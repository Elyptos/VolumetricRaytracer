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

#include "TextureFactory.h"
#include "Logger.h"

#ifdef _WIN32

#include "DXTextureCube.h"
#include "DXTexture3D.h"
#include "DirectXTex.h"
#include "DXRenderer.h"
#include <comdef.h>

VolumeRaytracer::VObjectPtr<VolumeRaytracer::VTextureCube> VolumeRaytracer::Renderer::VTextureFactory::LoadTextureCubeFromFile(std::weak_ptr<VRenderer> renderer, const std::wstring& path)
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
		VObjectPtr<DX::VDXTextureCube> res = VObject::CreateObject<DX::VDXTextureCube>();

		res->AssetPath = path;
		res->Width = metadata.width;
		res->Height = metadata.height;
		res->ArraySize = metadata.arraySize;
		res->DXTextureFormat = metadata.format;
		res->MipCount = metadata.mipLevels;
		res->RawImage = imageData;

		renderer.lock()->InitializeTexture(res);

		return res;
	}
	else
	{
		V_LOG_ERROR("Loaded texture is not a cubemap!");
		delete imageData;

		return nullptr;
	}
}

VolumeRaytracer::VObjectPtr<VolumeRaytracer::VTexture3D> VolumeRaytracer::Renderer::VTextureFactory::CreateTexture3D(std::weak_ptr<VRenderer> renderer, const size_t& width, const size_t& height, const size_t& depth, const size_t& mipLevels)
{
	VObjectPtr<VolumeRaytracer::VTexture3D> res = VObject::CreateObject<DX::VDXTexture3D>(width, height, depth, mipLevels);

	renderer.lock()->InitializeTexture(res);

	return res;
}

#endif