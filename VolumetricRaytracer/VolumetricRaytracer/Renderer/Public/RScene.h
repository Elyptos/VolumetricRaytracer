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

#include "Scene.h"

namespace VolumeRaytracer
{
	namespace Renderer
	{
		class VRenderer;

		class VRScene
		{
		public:
			VRScene() = default;
			virtual ~VRScene();

			virtual void InitFromScene(std::weak_ptr<VRenderer> renderer, std::weak_ptr<Scene::VScene> scene);
			virtual void SyncWithScene(std::weak_ptr<VRenderer> renderer, std::weak_ptr<Scene::VScene> scene) {}
			virtual void PrepareForRendering(std::weak_ptr<VRenderer> renderer) = 0;
			virtual void Cleanup();
		};
	}
}