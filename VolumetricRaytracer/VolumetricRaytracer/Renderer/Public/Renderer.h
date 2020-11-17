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
	namespace Voxel
	{
		class VVoxelScene;
	}

	namespace Renderer
	{
		class VRenderer
		{
		public:
			virtual ~VRenderer() = default;

			virtual void Render() = 0;
			virtual void Start() = 0;
			virtual void Stop() = 0;

			virtual bool IsActive() const = 0;
			virtual void SetSceneToRender(VObjectPtr<Voxel::VVoxelScene> scene);

		protected:
			std::weak_ptr<Voxel::VVoxelScene> SceneRef;
		};
	}
}