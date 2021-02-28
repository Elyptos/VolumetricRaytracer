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

#include "Material.h"
#include "StringHelpers.h"
#include <boost/filesystem.hpp>

std::shared_ptr<VolumeRaytracer::VSerializationArchive> VolumeRaytracer::VMaterial::Serialize() const
{
	std::shared_ptr<VSerializationArchive> res = std::make_shared<VSerializationArchive>();
	res->BufferSize = 0;
	res->Buffer = nullptr;

	res->Properties["Color"] = VSerializationArchive::From<VColor>(&AlbedoColor);
	res->Properties["Roughness"] = VSerializationArchive::From<float>(&Roughness);
	res->Properties["Metallic"] = VSerializationArchive::From<float>(&Metallic);
	res->Properties["TextureScale"] = VSerializationArchive::From<VVector2D>(&TextureScale);

	std::shared_ptr<VSerializationArchive> albedoArchive = std::make_shared<VSerializationArchive>();

	std::string str = VStringHelpers::WStringToString(AlbedoTexturePath);
	size_t numChars = str.size() + 1;

	albedoArchive->BufferSize = numChars;

	albedoArchive->Buffer = new char[albedoArchive->BufferSize];

	memcpy(albedoArchive->Buffer, str.c_str(), numChars);

	res->Properties["AlbedoTexture"] = albedoArchive;


	std::shared_ptr<VSerializationArchive> normalArchive = std::make_shared<VSerializationArchive>();

	str = VStringHelpers::WStringToString(NormalTexturePath);
	numChars = str.size() + 1;

	normalArchive->BufferSize = numChars;
	normalArchive->Buffer = new char[normalArchive->BufferSize];

	memcpy(normalArchive->Buffer, str.c_str(), numChars);

	res->Properties["NormalTexture"] = normalArchive;


	std::shared_ptr<VSerializationArchive> rmArchive = std::make_shared<VSerializationArchive>();

	str = VStringHelpers::WStringToString(AlbedoTexturePath);
	numChars = str.size() + 1;

	rmArchive->BufferSize = numChars;
	rmArchive->Buffer = new char[rmArchive->BufferSize];

	memcpy(rmArchive->Buffer, str.c_str(), numChars);

	res->Properties["RMTexture"] = rmArchive;

	return res;
}

void VolumeRaytracer::VMaterial::Deserialize(const std::wstring& sourcePath, std::shared_ptr<VSerializationArchive> archive)
{
	AlbedoColor = archive->Properties["Color"]->To<VColor>();
	Roughness = archive->Properties["Roughness"]->To<float>();
	Metallic = archive->Properties["Metallic"]->To<float>();
	TextureScale = archive->Properties["TextureScale"]->To<VVector2D>();
	
	AlbedoTexturePath = VStringHelpers::StringToWString(std::string(archive->Properties["AlbedoTexture"]->Buffer));
	NormalTexturePath = VStringHelpers::StringToWString(std::string(archive->Properties["NormalTexture"]->Buffer));
	RMTexturePath = VStringHelpers::StringToWString(std::string(archive->Properties["RMTexture"]->Buffer));

	boost::filesystem::path albedoBoostPath(AlbedoTexturePath);
	boost::filesystem::path normalBoostPath(NormalTexturePath);
	boost::filesystem::path rmTextureBoostPath(RMTexturePath);

	boost::filesystem::path sourceFolder = boost::filesystem::path(sourcePath).root_directory();

	if (HasAlbedoTexture() && !albedoBoostPath.is_absolute())
	{
		AlbedoTexturePath = (sourceFolder / albedoBoostPath).wstring();
	}

	if (HasNormalTexture() && !normalBoostPath.is_absolute())
	{
		NormalTexturePath = (sourceFolder / normalBoostPath).wstring();
	}

	if (HasRMTexture() && !rmTextureBoostPath.is_absolute())
	{
		RMTexturePath = (sourceFolder / rmTextureBoostPath).wstring();
	}
}

bool VolumeRaytracer::VMaterial::HasAlbedoTexture() const
{
	return AlbedoTexturePath != L"";
}

bool VolumeRaytracer::VMaterial::HasNormalTexture() const
{
	return NormalTexturePath != L"";
}

bool VolumeRaytracer::VMaterial::HasRMTexture() const
{
	return RMTexturePath != L"";
}
