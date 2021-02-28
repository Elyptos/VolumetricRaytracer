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

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <boost/filesystem.hpp>
#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/Deserialize.h>

#include "Scene.h"
#include "SceneInfo.h"
#include "SceneConverter.h"
#include "GLTFImporter.h"
#include "TextureLibraryImporter.h"
#include "FileStreamReader.h"
#include "VolumeConverter.h"
#include "SerializationManager.h"


int main(int argc, char** args)
{
	if (argc < 2)
	{
		std::cout << "Usage: Voxelizer.exe path/to/gltf/file [path/to/texture/lib]" << std::endl;
		return 0;
	}

	std::string filePath = std::string(args[1]);
	std::unique_ptr<VolumeRaytracer::Voxelizer::VFileStreamReader> fileStreamReader = std::make_unique<VolumeRaytracer::Voxelizer::VFileStreamReader>(boost::filesystem::current_path().string());

	if (!boost::filesystem::exists(filePath))
	{
		std::cerr << "GLTF file not found! Path: " << filePath << std::endl;
		return 1;
	}

	VolumeRaytracer::Voxelizer::VTextureLibrary textureLib;

	if (argc > 2)
	{
		textureLib = VolumeRaytracer::Voxelizer::VTextureLibraryImporter::Import(std::string(args[2]));
	}

	std::shared_ptr<std::istream> fileStream = fileStreamReader->GetInputStream(filePath);
	std::unique_ptr<Microsoft::glTF::GLTFResourceReader> gltfResourceReader = std::make_unique<Microsoft::glTF::GLTFResourceReader>(std::move(fileStreamReader));

	std::stringstream manifestStream;
	manifestStream << fileStream->rdbuf();

	std::string manifest = manifestStream.str();

	std::cout << "Starting gltf import" << std::endl;

	std::shared_ptr<VolumeRaytracer::Voxelizer::VSceneInfo> sceneInfo = nullptr;

	try
	{
		Microsoft::glTF::Document document = Microsoft::glTF::Deserialize(manifest);

		sceneInfo = VolumeRaytracer::Voxelizer::VGLTFImporter::ImportScene(&document, gltfResourceReader.get());
	}
	catch (const Microsoft::glTF::GLTFException& ex)
	{
		std::cerr << "Failed to read gltf file! " << ex.what() << std::endl;

		return 1;
	}

	if (sceneInfo == nullptr)
	{
		std::cerr << "Scene import failed! " << std::endl;
		return 1;
	}

	std::cout << "Gltf import finished" << std::endl;

	if (sceneInfo->Objects.size() == 0)
	{
		std::cerr << "Scene has no object, exiting! " << std::endl;
		return 1;
	}

	if (sceneInfo->Meshes.size() == 0)
	{
		std::cerr << "Scene has no meshes, exiting! " << std::endl;
		return 1;
	}

	VolumeRaytracer::VObjectPtr<VolumeRaytracer::Scene::VScene> scene = VolumeRaytracer::Voxelizer::VSceneConverter::ConvertSceneInfoToScene(*sceneInfo, textureLib);

	std::stringstream outputFileName;
	outputFileName << boost::filesystem::path(filePath).stem().string() << ".vox";

	boost::filesystem::path outputPath = boost::filesystem::path(filePath).parent_path() / outputFileName.str();

	std::cout << "Saving to file: " << boost::filesystem::absolute(outputPath).string();

	VolumeRaytracer::VSerializationManager::SaveToFile(scene, outputPath.string());

	return 0;
}