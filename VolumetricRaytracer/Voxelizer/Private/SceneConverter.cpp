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

#include "SceneConverter.h"
#include "VoxelVolume.h"
#include "Scene.h"
#include "VoxelObject.h"
#include "VolumeConverter.h"
#include <iostream>
#include <string>

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Scene::VScene> VolumeRaytracer::Voxelizer::VSceneConverter::ConvertSceneInfoToScene(const VSceneInfo& sceneInfo)
{
	VObjectPtr<Scene::VScene> scene = VObject::CreateObject<Scene::VScene>();

	std::cout << "Convert imported scene to voxel scene" << std::endl;

	std::cout << "Converting meshes to voxel volumes" << std::endl;

	boost::unordered_map<std::string, VolumeRaytracer::VObjectPtr<VolumeRaytracer::Voxel::VVoxelVolume>> volumes;

	for (const auto& mesh : sceneInfo.Meshes)
	{
		volumes[mesh.first] = VVolumeConverter::ConvertMeshInfoToVoxelVolume(mesh.second);
	}

	std::cout << "Converting scene objects" << std::endl;

	for (const auto& object : sceneInfo.Objects)
	{
		VObjectPtr<Scene::VVoxelObject> obj = scene->SpawnObject<Scene::VVoxelObject>(object.Position, object.Rotation, object.Scale);

		obj->SetVoxelVolume(volumes[object.MeshID]);
	}

	std::cout << "Scene conversion finished" << std::endl;

	return scene;
}
