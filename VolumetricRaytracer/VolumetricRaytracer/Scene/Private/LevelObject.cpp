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

#include "LevelObject.h"
#include "Logger.h"

std::weak_ptr<VolumeRaytracer::Scene::VScene> VolumeRaytracer::Scene::VLevelObject::GetScene() const
{
	return Scene;
}

void VolumeRaytracer::Scene::VLevelObject::SetScene(std::weak_ptr<VScene> scene)
{
	if (Scene.expired())
	{
		Scene = scene;
		OnSceneSet();
	}
	else
	{
		V_LOG_ERROR("Tried to change scene of object which is inside a scene!");
	}
}

void VolumeRaytracer::Scene::VLevelObject::RemoveFromScene()
{
	if (!Scene.expired())
	{
		OnPendingSceneRemoval();
		Scene.reset();
	}
}
