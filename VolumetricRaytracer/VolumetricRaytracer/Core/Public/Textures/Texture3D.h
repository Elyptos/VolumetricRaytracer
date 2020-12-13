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

#include "Texture.h"

namespace VolumeRaytracer
{
	class VTexture3D : public VTexture
	{
	public:
		virtual ~VTexture3D() = default;

		virtual void GetPixels(const size_t& mipLevel, uint8_t*& outPixelArray, size_t* outArraySize) = 0;

		size_t GetWidth() const;
		size_t GetHeight() const;
		size_t GetDepth() const;

	protected:
		size_t Width;
		size_t Height;
		size_t Depth;
	};
}