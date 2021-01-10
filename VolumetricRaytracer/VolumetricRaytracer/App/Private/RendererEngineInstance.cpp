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
		OnAxisInpuHandle.disconnect();
		Window = nullptr;
	}
}

void VolumeRaytracer::App::RendererEngineInstance::OnEngineUpdate(const float& deltaTime)
{
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

	//LoadSceneFromFile("../Voxelizer/Monkey.vox");
	Camera = Scene->SpawnObject<Scene::VCamera>(VVector(300.f, 0.f, 100.f), VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, 180.f * (M_PI / 180.f)), VVector::ONE);

	DirectionalLight = Scene->SpawnObject<Scene::VLight>(VVector::ZERO, VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::RIGHT, 45.f * (M_PI / 180.f))
		* VolumeRaytracer::VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, 135.f * (M_PI / 180.f)), VVector::ONE);
	DirectionalLight->IlluminationStrength = 5.f;

	Snowman = Scene->SpawnObject<Scene::VVoxelObject>(VVector::ZERO, VQuat::IDENTITY, VVector::ONE);
	Floor = Scene->SpawnObject<Scene::VVoxelObject>(VVector(0.f, 0.f, -80.f), VQuat::IDENTITY, VVector(10.f, 10.f, 0.25f));
	Sphere = Scene->SpawnObject<Scene::VVoxelObject>(VVector::ZERO, VQuat::IDENTITY, VVector::ONE);
	Cube = Scene->SpawnObject<Scene::VVoxelObject>(VVector(100.f, 100.f, 0.f), VQuat::IDENTITY, VVector::ONE);

	Scene->SetEnvironmentTexture(VolumeRaytracer::Renderer::VTextureFactory::LoadTextureCubeFromFile(Engine->GetRenderer(), L"Resources/Skybox/Skybox.dds"));
	Scene->SetActiveSceneCamera(Camera);
	Scene->SetActiveDirectionalLight(DirectionalLight);

	InitSnowmanObject();
	//InitFloor();
	//InitSphere();
	//InitCube();
}

void VolumeRaytracer::App::RendererEngineInstance::InitSnowmanObject()
{
	VObjectPtr<Voxel::VVoxelVolume> voxelVolume = VObject::CreateObject<Voxel::VVoxelVolume>(140, 200.f);

	VObjectPtr<Scene::VDensityGenerator> densityObj = VObject::CreateObject<Scene::VDensityGenerator>();

	std::shared_ptr<Scene::VSphere> sphere = std::make_shared<Scene::VSphere>();
	std::shared_ptr<Scene::VSphere> sphere2 = std::make_shared<Scene::VSphere>();
	std::shared_ptr<Scene::VSphere> sphere3 = std::make_shared<Scene::VSphere>();
	std::shared_ptr<Scene::VCylinder> cyliner1 = std::make_shared<Scene::VCylinder>();
	std::shared_ptr<Scene::VCylinder> cylinder2 = std::make_shared<Scene::VCylinder>();

	sphere->Radius = 70.f;
	sphere2->Radius = 50.f;
	sphere2->Position = VVector::FORWARD * 80.f;
	sphere3->Radius = 35.f;
	sphere3->Position = VVector::FORWARD * 70.f;
	cyliner1->Radius = 50.f;
	cyliner1->Height = 8.f;
	cyliner1->Position = VVector::FORWARD * 30.f;
	cyliner1->Rotation = VQuat::FromAxisAngle(VolumeRaytracer::VVector::UP, 90.f * (M_PI / 180.f));
	cylinder2->Radius = 25.f;
	cylinder2->Height = 30.f;
	cylinder2->Position = VVector::RIGHT * -30.f;

	densityObj->GetRootShape().AddChild(sphere)
		.AddChild(sphere2)
		.AddChild(sphere3)
		.AddChild(cyliner1)
		.AddChild(cylinder2);

	std::shared_ptr<Scene::VBox> box = std::make_shared<Scene::VBox>();
	box->Extends = VVector::ONE * 100.f;

	densityObj->Position = VVector::FORWARD * -150.f;

	#pragma omp parallel for collapse(3)
	for (int x = 0; x < voxelVolume->GetSize(); x++)
	{
		for (int y = 0; y < voxelVolume->GetSize(); y++)
		{
			for (int z = 0; z < voxelVolume->GetSize(); z++)
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

	Snowman->SetVoxelVolume(voxelVolume);
}

void VolumeRaytracer::App::RendererEngineInstance::InitFloor()
{
	VObjectPtr<Voxel::VVoxelVolume> voxelVolume = VObject::CreateObject<Voxel::VVoxelVolume>(2, 200.f);

	#pragma omp parallel for collapse(3)
	for (int x = 0; x < voxelVolume->GetSize(); x++)
	{
		for (int y = 0; y < voxelVolume->GetSize(); y++)
		{
			for (int z = 0; z < voxelVolume->GetSize(); z++)
			{
				VIntVector voxelIndex = VIntVector(x, y, z);

				VVector voxelPos = voxelVolume->VoxelIndexToRelativePosition(voxelIndex);

				Voxel::VVoxel voxel;

				voxel.Material = 1;
				voxel.Density = -1.f;

				voxelVolume->SetVoxel(voxelIndex, voxel);
			}
		}
	}

	Floor->SetVoxelVolume(voxelVolume);
}

void VolumeRaytracer::App::RendererEngineInstance::InitSphere()
{
	VObjectPtr<Voxel::VVoxelVolume> voxelVolume = VObject::CreateObject<Voxel::VVoxelVolume>(50, 100.f);

	VObjectPtr<Scene::VDensityGenerator> densityObj = VObject::CreateObject<Scene::VDensityGenerator>();

	std::shared_ptr<Scene::VSphere> sphere = std::make_shared<Scene::VSphere>();

	sphere->Radius = 40.f;

	densityObj->GetRootShape().AddChild(sphere);

#pragma omp parallel for collapse(3)
	for (int x = 0; x < voxelVolume->GetSize(); x++)
	{
		for (int y = 0; y < voxelVolume->GetSize(); y++)
		{
			for (int z = 0; z < voxelVolume->GetSize(); z++)
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

	Sphere->SetVoxelVolume(voxelVolume);
}

void VolumeRaytracer::App::RendererEngineInstance::InitCube()
{
	VObjectPtr<Voxel::VVoxelVolume> voxelVolume = VObject::CreateObject<Voxel::VVoxelVolume>(50, 100.f);

	VObjectPtr<Scene::VDensityGenerator> densityObj = VObject::CreateObject<Scene::VDensityGenerator>();

	std::shared_ptr<Scene::VBox> box = std::make_shared<Scene::VBox>();

	box->Extends = VVector::ONE * 40.f;

	densityObj->GetRootShape().AddChild(box);

	#pragma omp parallel for collapse(3)
	for (int x = 0; x < voxelVolume->GetSize(); x++)
	{
		for (int y = 0; y < voxelVolume->GetSize(); y++)
		{
			for (int z = 0; z < voxelVolume->GetSize(); z++)
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

	Cube->SetVoxelVolume(voxelVolume);
}

void VolumeRaytracer::App::RendererEngineInstance::LoadSceneFromFile(const std::string& filePath)
{
	Scene = VSerializationManager::LoadFromFile<Scene::VScene>(filePath);
}
