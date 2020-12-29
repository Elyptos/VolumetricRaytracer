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

#pragma once

#include "IEngineInstance.h"
#include "Object.h"
#include "Vector.h"
#include <boost/signals2/connection.hpp>
#include <boost/bind.hpp>

namespace VolumeRaytracer
{
	namespace UI
	{
		class VWindow;
		enum class EVKeyType;
		enum class EVAxisType;
	}

	namespace Scene
	{
		class VScene;
		class VVoxelObject;
		class VLight;
		class VCamera;
	}

	namespace App
	{
		class RendererEngineInstance : public Engine::IEngineInstance
		{
		public:
			void OnEngineInitialized(Engine::VEngine* owningEngine) override;
			void OnEngineShutdown() override;
			void OnEngineUpdate(const float& deltaTime) override;

		private:
			void CreateRendererWindow();

			void OnWindowClosed();
			void OnKeyDown(const VolumeRaytracer::UI::EVKeyType& key);
			void OnAxisInput(const VolumeRaytracer::UI::EVAxisType& axis, const float& delta);

			void InitScene();

			void InitSnowmanObject();
			void InitFloor();
			void InitSphere();
			void InitCube();

			void LoadSceneFromFile(const std::string& filePath);

		private:
			Engine::VEngine* Engine = nullptr;
			VObjectPtr<UI::VWindow> Window = nullptr;
			VObjectPtr<Scene::VScene> Scene = nullptr;

			VObjectPtr<Scene::VCamera> Camera = nullptr;
			VObjectPtr<Scene::VLight> DirectionalLight = nullptr;
			
			VObjectPtr<Scene::VVoxelObject> Snowman = nullptr;
			VObjectPtr<Scene::VVoxelObject> Floor = nullptr;
			VObjectPtr<Scene::VVoxelObject> Sphere = nullptr;
			VObjectPtr<Scene::VVoxelObject> Cube = nullptr;

			boost::signals2::connection OnWindowClosedHandle;
			boost::signals2::connection OnKeyDownHandle;
			boost::signals2::connection OnAxisInpuHandle;
		};
	}
}