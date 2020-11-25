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
#include "WindowRenderTargetFactory.h"
#include "Window.h"
#include "Engine.h"
#include "Renderer.h"
#include "Camera.h"
#include "Vector.h"
#include "Quat.h"
#include "VoxelScene.h"

#define _USE_MATH_DEFINES

#include <math.h>

void VolumeRaytracer::App::RendererEngineInstance::OnEngineInitialized(Engine::VEngine* owningEngine)
{
	Engine = owningEngine;

	CreateRendererWindow();
	Window->Show();

	VObjectPtr<Renderer::VRenderTarget> renderTarget = UI::VWindowRenderTargetFactory::CreateRenderTarget(Engine->GetRenderer(), Window);
	Window->SetRenderTarget(renderTarget);

	InitScene();

	std::shared_ptr<Renderer::VRenderer> renderer = Engine->GetRenderer().lock();

	renderer->SetSceneToRender(Scene);
}

void VolumeRaytracer::App::RendererEngineInstance::OnEngineShutdown()
{
	Engine = nullptr;
	
	if (Window)
	{
		Window->Close();

		OnWindowClosedHandle.disconnect();
		Window = nullptr;
	}
}

void VolumeRaytracer::App::RendererEngineInstance::OnEngineUpdate(const float& deltaTime)
{
	float rotationAmount = 20.f * deltaTime;

	VolumeRaytracer::VQuat currentRotation = Scene->GetSceneCamera()->Rotation;

	currentRotation = currentRotation * VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, rotationAmount * (M_PI / 180.f));

	Scene->GetSceneCamera()->Rotation = currentRotation;

	std::wstringstream windowTitle;

	windowTitle << L"Volume Raytracer | FPS: " << Engine->GetFPS();
	
	Window->SetTitle(windowTitle.str());
}

void VolumeRaytracer::App::RendererEngineInstance::CreateRendererWindow()
{
	Window = VolumeRaytracer::UI::VWindowFactory::NewWindow();
	OnWindowClosedHandle = Window->OnWindowClosed_Bind(boost::bind(&RendererEngineInstance::OnWindowClosed, this));
}

void VolumeRaytracer::App::RendererEngineInstance::OnWindowClosed()
{
	if (Engine != nullptr)
	{
		Engine->Shutdown();
	}
}

void VolumeRaytracer::App::RendererEngineInstance::InitScene()
{
	Scene = VObject::CreateObject<Voxel::VVoxelScene>(1, 50.f);
	Scene->GetSceneCamera()->Position = VolumeRaytracer::VVector(300.f, 0.f, 100.f);
	//Scene->GetSceneCamera()->Rotation = VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::RIGHT, 180.f * (M_PI / 180.f));

	Scene->SetPathToEnvironmentMap(L"Resources/Skybox/Skybox.dds");

	Voxel::VVoxel voxelToUse;
	voxelToUse.Material = 1;

	Scene->SetVoxel(0, 0, 0, voxelToUse);
}

