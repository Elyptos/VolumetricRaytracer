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
#include "Color.h"
#include <string>
#include "ISerializable.h"

namespace VolumeRaytracer
{
	struct VMaterial : IVSerializable
	{
	public:
		VColor AlbedoColor = VColor(0.8f, 0.8f, 0.8f, 1.f);
		float Roughness = 0.8f;
		float Metallic = 0.f;

		std::wstring AlbedoTexturePath = L"";
		std::wstring NormalTexturePath = L"";
		std::wstring RMTexturePath = L"";

		VVector2D TextureScale = VVector2D(100.f, 100.f);

	public:
		std::shared_ptr<VSerializationArchive> Serialize() const override;
		void Deserialize(const std::wstring& sourcePath, std::shared_ptr<VSerializationArchive> archive) override;

		bool HasAlbedoTexture() const;
		bool HasNormalTexture() const;
		bool HasRMTexture() const;
	};
}