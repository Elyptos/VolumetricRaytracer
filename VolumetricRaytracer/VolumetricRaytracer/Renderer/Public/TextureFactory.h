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
#include <string>

namespace VolumeRaytracer
{
	class VTexture;
	class VTexture3D;
	class VTexture3DFloat;
	class VTextureCube;

	namespace Renderer
	{
		class VRenderer;
		
		class VTextureFactory
		{
		public:
			static VObjectPtr<VTextureCube> LoadTextureCubeFromFile(std::weak_ptr<VRenderer> renderer, const std::wstring& path);
			
			static VObjectPtr<VTexture3D> CreateTexture3D(std::weak_ptr<VRenderer> renderer, const size_t& width, const size_t& height, const size_t& depth, const size_t& mipLevels);
			static VObjectPtr<VTexture3DFloat> CreateTexture3DFloat(std::weak_ptr<VRenderer> renderer, const size_t& width, const size_t& height, const size_t& depth, const size_t& mipLevels);
		};
	}
}