/*
	Copyright (c) 2021 Thomas Schöngrundner

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
*/

#include "TextureLibraryImporter.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <rapidjson/document.h>
#include <codecvt>

VolumeRaytracer::Voxelizer::VTextureLibrary VolumeRaytracer::Voxelizer::VTextureLibraryImporter::Import(const std::string filePath)
{
	VTextureLibrary textureLib;

	if (!boost::filesystem::exists(filePath))
	{
		std::cerr << "Texture library file not found! Path: " << filePath << std::endl;
		return textureLib;
	}

	std::string content;
	std::string line;

	std::ifstream file(filePath);

	if (file.is_open())
	{
		while(std::getline(file, line))
		{
			content += line;
		}
	}

	file.close();

	rapidjson::Document document;

	if (!document.Parse(content.c_str()).HasParseError())
	{
		if (document.IsObject())
		{
			if (document.HasMember("materials") && document["materials"].IsArray())
			{
				auto materialsArray = document["materials"].GetArray();

				for (const auto& elem : materialsArray)
				{
					if (elem.HasMember("material") && elem["material"].IsString())
					{
						VMaterialTextures textures;

						if (elem.HasMember("tiling-x") && elem["tiling-x"].IsNumber())
						{
							textures.TextureTiling.X = elem["tiling-x"].GetFloat();
						}

						if (elem.HasMember("tiling-y") && elem["tiling-y"].IsNumber())
						{
							textures.TextureTiling.Y = elem["tiling-y"].GetFloat();
						}

						if (elem.HasMember("albedo") && elem["albedo"].IsString())
						{
							textures.Albedo = StringToWString(std::string(elem["albedo"].GetString()));
						}

						if (elem.HasMember("normal") && elem["normal"].IsString())
						{
							textures.Normal = StringToWString(std::string(elem["normal"].GetString()));
						}

						if (elem.HasMember("rm") && elem["rm"].IsString())
						{
							textures.RM = StringToWString(std::string(elem["rm"].GetString()));
						}

						textureLib.Materials[std::string(elem["material"].GetString())] = textures;
					}
				}
			}
		}
	}

	return textureLib;
}

std::wstring VolumeRaytracer::Voxelizer::VTextureLibraryImporter::StringToWString(const std::string& str)
{
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;

	return converter.from_bytes(str);
}
