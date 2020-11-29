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

#include "RScene.h"

VolumeRaytracer::Renderer::VRScene::~VRScene()
{}

void VolumeRaytracer::Renderer::VRScene::InitFromScene(Voxel::VVoxelScene* scene)
{
	Cleanup();

	Bounds = scene->GetSceneBounds();
	VolumeExtends = scene->GetVolumeExtends();
	VoxelCountAlongAxis = scene->GetSize();
}

void VolumeRaytracer::Renderer::VRScene::SyncWithScene(Voxel::VVoxelScene* scene)
{
	
}

void VolumeRaytracer::Renderer::VRScene::Cleanup()
{
}
