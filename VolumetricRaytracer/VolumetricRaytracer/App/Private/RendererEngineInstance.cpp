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

#define _USE_MATH_DEFINES

#include <math.h>
#include "SerializationManager.h"

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
		OnKeyPressedHandle.disconnect();
		OnAxisInpuHandle.disconnect();
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

	Sphere1Rotation += (10.f * deltaTime);
	Sphere2Rotation -= (50.f * deltaTime);

	if (Sphere1Rotation > 360.f)
	{
		Sphere1Rotation = Sphere1Rotation - 360.f;
	}

	if (Sphere2Rotation < 0.f)
	{
		Sphere2Rotation = Sphere2Rotation + 360.f;
	}

	VQuat rotationQuat = VQuat::FromAxisAngle(VVector::UP, Sphere1Rotation * (M_PI / 180.f));

	Sphere1->Position = rotationQuat * Sphere1RelPosition;

	rotationQuat = VQuat::FromAxisAngle(VVector::UP, Sphere2Rotation * (M_PI / 180.f));

	Sphere2->Position = rotationQuat * Sphere2relPosition;
}

void VolumeRaytracer::App::RendererEngineInstance::CreateRendererWindow()
{
	Window = VolumeRaytracer::UI::VWindowFactory::NewWindow();
	OnWindowClosedHandle = Window->OnWindowClosed_Bind(boost::bind(&RendererEngineInstance::OnWindowClosed, this));

	OnKeyDownHandle = Window->OnKeyDown_Bind(boost::bind(&RendererEngineInstance::OnKeyDown, this, boost::placeholders::_1));
	OnKeyPressedHandle = Window->OnKeyPressed_Bind(boost::bind(&RendererEngineInstance::OnKeyPressed, this, boost::placeholders::_1));
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

	VolumeRaytracer::VVector forward = Camera->Rotation.GetForwardVector();
	VolumeRaytracer::VVector right = Camera->Rotation.GetRightVector();

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

	Camera->Position = Camera->Position + velocity;
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

void VolumeRaytracer::App::RendererEngineInstance::OnAxisInput(const VolumeRaytracer::UI::EVAxisType& axis, const float& delta)
{
	if (Scene != nullptr)
	{
		if (axis == VolumeRaytracer::UI::EVAxisType::MouseX)
		{
			VolumeRaytracer::VQuat deltaRotation = VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, -delta * (M_PI / 180.f));

			Camera->Rotation = deltaRotation * Camera->Rotation;
		}
		else if (axis == VolumeRaytracer::UI::EVAxisType::MouseY)
		{
			VolumeRaytracer::VQuat currentRotation = Camera->Rotation;

			VolumeRaytracer::VQuat deltaRotation = VolumeRaytracer::VQuat::FromAxisAngle(currentRotation.GetRightVector(), delta * (M_PI / 180.f));

			Camera->Rotation = deltaRotation * currentRotation;
		}
	}
}

void VolumeRaytracer::App::RendererEngineInstance::InitScene()
{
	Scene = VObject::CreateObject<Scene::VScene>();

	LoadSceneFromFile(L"Resources/Model/Monkey.vox");
	Camera = Scene->SpawnObject<Scene::VCamera>(VVector(300.f, 0.f, 100.f), VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, 180.f * (M_PI / 180.f)), VVector::ONE);

	DirectionalLight = Scene->SpawnObject<Scene::VLight>(VVector::ZERO, VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, 45.f * (M_PI / 180.f))
		* VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::RIGHT, -30.f * (M_PI / 180.f)), VVector::ONE);
	DirectionalLight->IlluminationStrength = 6.f;

	Scene->SetEnvironmentTexture(VolumeRaytracer::Renderer::VTextureFactory::LoadTextureCubeFromFile(Engine->GetRenderer(), L"Resources/Skybox/Skybox.dds"));
	Scene->SetActiveSceneCamera(Camera);
	Scene->SetActiveDirectionalLight(DirectionalLight);

	Sphere1RelPosition = VVector(200.f, 0.f, 100.f);
	Sphere2relPosition = VVector(100.f, 0.f, 200.f);

	VMaterial material;

	material.AlbedoColor = VColor::RED;
	material.Roughness = 0.1f;
	material.Metallic = 0.6f;


	Sphere1 = InitSphere(40.f, material);
	Sphere1->Position = Sphere1RelPosition;

	material.AlbedoColor = VColor::BLUE;

	Sphere2 = InitSphere(20.f, material);
	Sphere2->Position = Sphere2relPosition;
}

VolumeRaytracer::VObjectPtr<VolumeRaytracer::Scene::VVoxelObject> VolumeRaytracer::App::RendererEngineInstance::InitSphere(const float& radius, const VMaterial& material)
{
	VObjectPtr<Voxel::VVoxelVolume> voxelVolume = VObject::CreateObject<Voxel::VVoxelVolume>(6, 100.f);

	Voxel::VVoxel voxel;
	voxel.Material = 1;
	voxel.Density = -10;

	VObjectPtr<Scene::VDensityGenerator> densityObj = VObject::CreateObject<Scene::VDensityGenerator>();

	std::shared_ptr<Scene::VSphere> sphere = std::make_shared<Scene::VSphere>();

	sphere->Radius = radius;

	densityObj->GetRootShape().AddChild(sphere);

	size_t voxelCount = voxelVolume->GetSize();

	size_t volumeSize = voxelCount * voxelCount * voxelCount;

#pragma omp parallel for collapse(3)
	for (int x = 0; x < voxelCount; x++)
	{
		for (int y = 0; y < voxelCount; y++)
		{
			for (int z = 0; z < voxelCount; z++)
			{
				VIntVector voxelIndex = VIntVector(x, y, z);

				VVector voxelPos = voxelVolume->VoxelIndexToRelativePosition(voxelIndex);

				float density = densityObj->Evaluate(voxelPos);

				Voxel::VVoxel voxel;

				voxel.Material = density <= 0 ? 1 : 0;
				voxel.Density = density;

				voxelVolume->SetVoxel(voxelIndex, voxel);
			}
		}
	}

	voxelVolume->SetMaterial(material);

	VObjectPtr<Scene::VVoxelObject> sphereObj = Scene->SpawnObject<Scene::VVoxelObject>(VVector::ZERO, VQuat::IDENTITY, VVector::ONE);

	sphereObj->SetVoxelVolume(voxelVolume);

	return sphereObj;
}

void VolumeRaytracer::App::RendererEngineInstance::LoadSceneFromFile(const std::wstring& filePath)
{
	Scene = VSerializationManager::LoadFromFile<Scene::VScene>(filePath);
}
