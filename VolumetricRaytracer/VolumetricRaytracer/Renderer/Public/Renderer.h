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

#include "Object.h"
#include <memory>

namespace VolumeRaytracer
{
	class VTextureCube;
	class VTexture;

	namespace Scene
	{
		class VScene;
	}

	namespace Renderer
	{
		enum class EVRenderMode
		{
			Interp = 0,
			Interp_Unlit = 1,
			Interp_NoTex = 2,
			Interp_NoTex_Unlit = 3,
			Cube = 4,
			Cube_Unlit = 5,
			Cube_NoTex = 6,
			Cube_NoTex_Unlit = 7
		};

		class VRenderer : public std::enable_shared_from_this<VRenderer>
		{
		public:
			virtual ~VRenderer() = default;

			virtual void Render() = 0;
			virtual bool Start() = 0;
			virtual void Stop() = 0;

			virtual bool IsActive() const = 0;
			virtual void SetSceneToRender(VObjectPtr<Scene::VScene> scene);

			virtual void InitializeTexture(VObjectPtr<VTexture> texture) = 0;
			virtual void UploadToGPU(VObjectPtr<VTexture> texture) = 0;

			virtual void ResizeRenderOutput(unsigned int width, unsigned int height) = 0;

			void SetRendererMode(const EVRenderMode& renderMode);

		protected:
			std::weak_ptr<Scene::VScene> SceneRef;
			EVRenderMode RenderMode = EVRenderMode::Interp;
		};
	}
}