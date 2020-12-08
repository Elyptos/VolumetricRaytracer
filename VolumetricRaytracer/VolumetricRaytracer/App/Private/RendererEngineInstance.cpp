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
#include "Quat.h"
#include "VoxelScene.h"

#define _USE_MATH_DEFINES

#include <math.h>

void VolumeRaytracer::App::RendererEngineInstance::OnEngineInitialized(Engine::VEngine* owningEngine)
{
	Engine = owningEngine;

	CreateRendererWindow();
	Window->Show();
	Window->SetRenderer(Engine->GetRenderer());

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
		OnKeyDownHandle.disconnect();
		OnAxisInpuHandle.disconnect();
		Window = nullptr;
	}
}

void VolumeRaytracer::App::RendererEngineInstance::OnEngineUpdate(const float& deltaTime)
{
	//VVector currentPos = Scene->GetSceneCamera()->Position;
	//currentPos = VVector::Lerp(currentPos, TargetCameraLocation, deltaTime * 10.f);

	//Scene->GetSceneCamera()->Position = currentPos;

	std::wstringstream windowTitle;

	windowTitle << L"Volume Raytracer | FPS: " << Engine->GetFPS();
	
	Window->SetTitle(windowTitle.str());
}

void VolumeRaytracer::App::RendererEngineInstance::CreateRendererWindow()
{
	Window = VolumeRaytracer::UI::VWindowFactory::NewWindow();
	OnWindowClosedHandle = Window->OnWindowClosed_Bind(boost::bind(&RendererEngineInstance::OnWindowClosed, this));

	OnKeyDownHandle = Window->OnKeyDown_Bind(boost::bind(&RendererEngineInstance::OnKeyDown, this, boost::placeholders::_1));
	OnAxisInpuHandle = Window->OnAxisInput_Bind(boost::bind(&RendererEngineInstance::OnAxisInput, this, boost::placeholders::_1, boost::placeholders::_2));
}

void VolumeRaytracer::App::RendererEngineInstance::OnWindowClosed()
{
	if (Engine != nullptr)
	{
		Engine->Shutdown();
	}
}

void VolumeRaytracer::App::RendererEngineInstance::OnKeyDown(const VolumeRaytracer::UI::EVKeyType& key)
{
	if(Scene == nullptr || Engine == nullptr)
		return;

	const float movementSpeed = 100.f;
	VolumeRaytracer::VVector velocity;

	VolumeRaytracer::VVector forward = Scene->GetSceneCamera()->Rotation.GetForwardVector();
	VolumeRaytracer::VVector right = Scene->GetSceneCamera()->Rotation.GetRightVector();

	switch (key)
	{
		case VolumeRaytracer::UI::EVKeyType::W:
		{
			velocity = forward * movementSpeed * Engine->GetEngineDeltaTime();
		}
		break;
		case VolumeRaytracer::UI::EVKeyType::A:
		{
			velocity = right * movementSpeed * Engine->GetEngineDeltaTime();
		}
		break;
		case VolumeRaytracer::UI::EVKeyType::S:
		{
			velocity = forward * -movementSpeed * Engine->GetEngineDeltaTime();
		}
		break;
		case VolumeRaytracer::UI::EVKeyType::D:
		{
			velocity = right * -movementSpeed * Engine->GetEngineDeltaTime();
		}
		break;
	}

	Scene->GetSceneCamera()->Position = Scene->GetSceneCamera()->Position + velocity;
}

void VolumeRaytracer::App::RendererEngineInstance::OnAxisInput(const VolumeRaytracer::UI::EVAxisType& axis, const float& delta)
{
	if (Scene != nullptr)
	{
		if (axis == VolumeRaytracer::UI::EVAxisType::MouseX)
		{
			VolumeRaytracer::VQuat deltaRotation = VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, -delta * (M_PI / 180.f));

			Scene->GetSceneCamera()->Rotation = deltaRotation * Scene->GetSceneCamera()->Rotation;
		}
		else if (axis == VolumeRaytracer::UI::EVAxisType::MouseY)
		{
			VolumeRaytracer::VQuat currentRotation = Scene->GetSceneCamera()->Rotation;

			VolumeRaytracer::VQuat deltaRotation = VolumeRaytracer::VQuat::FromAxisAngle(currentRotation.GetRightVector(), delta * (M_PI / 180.f));

			Scene->GetSceneCamera()->Rotation = deltaRotation * currentRotation;
		}
	}
}

void VolumeRaytracer::App::RendererEngineInstance::InitScene()
{
	Scene = VObject::CreateObject<Voxel::VVoxelScene>(40, 200.f);
	Scene->GetSceneCamera()->Position = VolumeRaytracer::VVector(300.f, 0.f, 100.f);
	TargetCameraLocation = Scene->GetSceneCamera()->Position;
	Scene->GetSceneCamera()->Rotation = VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, 180.f * (M_PI / 180.f));

	Scene->SetEnvironmentTexture(VolumeRaytracer::Renderer::VTextureFactory::LoadTextureCubeFromFile(Engine->GetRenderer(), L"Resources/Skybox/Skybox.dds"));

	/*Voxel::VVoxel voxelToUse;
	voxelToUse.Material = 1;
	voxelToUse.Density = -1;

	Scene->SetVoxel(1, 1, 1, voxelToUse);*/
	//Scene->SetVoxel(6, 5, 5, voxelToUse);
	//Scene->SetVoxel(4, 5, 5, voxelToUse);
	//Scene->SetVoxel(5, 6, 5, voxelToUse);
	//Scene->SetVoxel(5, 4, 5, voxelToUse);
	//Scene->SetVoxel(5, 5, 6, voxelToUse);
	//Scene->SetVoxel(5, 5, 4, voxelToUse);

	for (unsigned int x = 0; x < Scene->GetSize(); x++)
	{
		for (unsigned int y = 0; y < Scene->GetSize(); y++)
		{
			for (unsigned int z = 0; z < Scene->GetSize(); z++)
			{
				VVector voxelPos = Scene->VoxelIndexToWorldPosition(x, y, z);

				float density = voxelPos.Length() - 80.f;

				if (density <= 0)
				{
					int i = 0;
				}

				Voxel::VVoxel voxel;

				voxel.Material = density <= 0 ? 1 : 0;
				voxel.Density = density;

				Scene->SetVoxel(x, y, z, voxel);
			}
		}
	}
}

