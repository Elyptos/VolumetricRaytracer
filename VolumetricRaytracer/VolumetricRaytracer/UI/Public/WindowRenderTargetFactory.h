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

#include <memory>
#include "Object.h"

namespace VolumeRaytracer
{
	namespace Renderer
	{
		class VRenderTarget;
		class VRenderer;
	}

	namespace UI
	{
		class VWindow;

		class VWindowRenderTargetFactory
		{
		public:
			static VObjectPtr<Renderer::VRenderTarget> CreateRenderTarget(std::weak_ptr<Renderer::VRenderer> renderer, std::weak_ptr<VWindow> window);
		};
	}
}