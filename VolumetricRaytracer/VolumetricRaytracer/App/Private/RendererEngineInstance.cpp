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

#include "../Public/RendererEngineInstance.h"
#include "WindowFactory.h"
#include "TextureFactory.h"
#include "WindowRenderTargetFactory.h"
#include "Window.h"
#include "Engine.h"
#include "Renderer.h"
#include "Camera.h"
#include "Light.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Quat.h"
#include "Scene.h"
#include "VoxelObject.h"
#include "VoxelVolume.h"
#include "DensityGenerator.h"
#include "OpenFileDialog.h"
#include "MessageBox.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include "SerializationManager.h"

void VolumeRaytracer::App::RendererEngineInstance::OnEngineInitialized(Engine::VEngine* owningEngine)
{
	Engine = owningEngine;

	std::wstring filePath;

	if (UI::VOpenFileDialog::Open(L"Voxel File;*.vox", filePath))
	{
		CreateRendererWindow();
		Window->Show();
		Window->SetRenderer(Engine->GetRenderer());

		InitScene(filePath);

		std::shared_ptr<Renderer::VRenderer> renderer = Engine->GetRenderer().lock();

		renderer->SetSceneToRender(Scene);
	}
	else
	{
		Engine->Shutdown();
	}
}

void VolumeRaytracer::App::RendererEngineInstance::OnEngineShutdown()
{
	Engine = nullptr;
	
	if (Window)
	{
		Window->Close();

		OnWindowClosedHandle.disconnect();
		OnKeyPressedHandle.disconnect();
		Window = nullptr;
	}
}

void VolumeRaytracer::App::RendererEngineInstance::OnEngineUpdate(const float& deltaTime)
{
	std::wstringstream windowTitle;

	windowTitle << L"Volume Raytracer | FPS: " << Engine->GetFPS();
	
	Window->SetTitle(windowTitle.str());
	
	std::shared_ptr<Renderer::VRenderer> renderer = Engine->GetRenderer().lock();

	if (!CubeMode && !Unlit && ShowTextures)
	{
		renderer->SetRendererMode(Renderer::EVRenderMode::Interp);
	}
	else if (!CubeMode && Unlit && ShowTextures)
	{
		renderer->SetRendererMode(Renderer::EVRenderMode::Interp_Unlit);
	}
	else if (!CubeMode && Unlit && !ShowTextures)
	{
		renderer->SetRendererMode(Renderer::EVRenderMode::Interp_NoTex_Unlit);
	}
	else if (!CubeMode && !Unlit && !ShowTextures)
	{
		renderer->SetRendererMode(Renderer::EVRenderMode::Interp_NoTex);
	}
	else if (CubeMode && !Unlit && ShowTextures)
	{
		renderer->SetRendererMode(Renderer::EVRenderMode::Cube);
	}
	else if (CubeMode && Unlit && ShowTextures)
	{
		renderer->SetRendererMode(Renderer::EVRenderMode::Cube_Unlit);
	}
	else if (CubeMode && Unlit && !ShowTextures)
	{
		renderer->SetRendererMode(Renderer::EVRenderMode::Cube_NoTex_Unlit);
	}
	else if (CubeMode && !Unlit && !ShowTextures)
	{
		renderer->SetRendererMode(Renderer::EVRenderMode::Cube_NoTex);
	}

	RotateScene(deltaTime);
}

void VolumeRaytracer::App::RendererEngineInstance::CreateRendererWindow()
{
	Window = VolumeRaytracer::UI::VWindowFactory::NewWindow();
	OnWindowClosedHandle = Window->OnWindowClosed_Bind(boost::bind(&RendererEngineInstance::OnWindowClosed, this));

	OnKeyPressedHandle = Window->OnKeyPressed_Bind(boost::bind(&RendererEngineInstance::OnKeyPressed, this, boost::placeholders::_1));
}

void VolumeRaytracer::App::RendererEngineInstance::OnWindowClosed()
{
	if (Engine != nullptr)
	{
		Engine->Shutdown();
	}
}

void VolumeRaytracer::App::RendererEngineInstance::OnKeyPressed(const VolumeRaytracer::UI::EVKeyType& key)
{
	switch (key)
	{
		case UI::EVKeyType::N1:
		{
			CubeMode = !CubeMode;
		}
		break;
		case UI::EVKeyType::N2:
		{
			ShowTextures = !ShowTextures;
		}
		break;
		case UI::EVKeyType::N3:
		{
			Unlit = !Unlit;
		}
		break;
	}
}

void VolumeRaytracer::App::RendererEngineInstance::InitScene(const std::wstring& filePath)
{
	Scene = VObject::CreateObject<Scene::VScene>();

	LoadSceneFromFile(filePath);

	std::vector<std::weak_ptr<Scene::VLevelObject>> objects = Scene->GetAllPlacedObjects();

	VAABB sceneBounds = Scene->GetSceneBounds();
	ScenePivot = sceneBounds.GetCenterPosition();

	for (auto& elem : objects)
	{
		std::shared_ptr<Scene::VLevelObject> obj = elem.lock();

		TransformCache[obj].RelativeRotation = obj->Rotation;
		TransformCache[obj].RelativePosition = obj->Position - ScenePivot;
	}

	float maxSceneSize = VMathHelpers::Max(sceneBounds.GetExtends().X, VMathHelpers::Max(sceneBounds.GetExtends().Y, sceneBounds.GetExtends().Z));

	VQuat camRotation = VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, 180.f * (M_PI / 180.f)) * VQuat::FromAxisAngle(VolumeRaytracer::VVector::RIGHT, 25.f * (M_PI / 180.f));
	VVector camPos = sceneBounds.GetCenterPosition() - camRotation.GetForwardVector() * (maxSceneSize + 100.f);

	Camera = Scene->SpawnObject<Scene::VCamera>(camPos, camRotation, VVector::ONE);

	DirectionalLight = Scene->SpawnObject<Scene::VLight>(VVector::ZERO, VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, 45.f * (M_PI / 180.f))
		* VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::RIGHT, -30.f * (M_PI / 180.f)), VVector::ONE);
	DirectionalLight->IlluminationStrength = 6.f;

	Scene->SetEnvironmentTexture(VolumeRaytracer::Renderer::VTextureFactory::LoadTextureCubeFromFile(Engine->GetRenderer(), L"Resources/Skybox/Skybox.dds"));
	Scene->SetActiveSceneCamera(Camera);
	Scene->SetActiveDirectionalLight(DirectionalLight);
}

void VolumeRaytracer::App::RendererEngineInstance::LoadSceneFromFile(const std::wstring& filePath)
{
	Scene = VSerializationManager::LoadFromFile<Scene::VScene>(filePath);
}

void VolumeRaytracer::App::RendererEngineInstance::RotateScene(const float& deltaTime)
{
	if (Scene != nullptr)
	{
		currentSceneRotation += (10.f * deltaTime);

		if (currentSceneRotation > 360.f)
		{
			currentSceneRotation = currentSceneRotation - 360.f;
		}

		VQuat rotationQuat = VQuat::FromAxisAngle(VVector::UP, currentSceneRotation * (M_PI / 180.f));

		std::vector<std::weak_ptr<Scene::VLevelObject>> objects = Scene->GetAllPlacedObjects();

		for (auto& elem : objects)
		{
			std::shared_ptr<Scene::VLevelObject> obj = elem.lock();

			if (TransformCache.find(obj) != TransformCache.end())
			{
				obj->Position = ScenePivot + (rotationQuat * TransformCache[obj].RelativePosition);
				obj->Rotation = TransformCache[obj].RelativeRotation * rotationQuat;
			}
		}
	}
}
